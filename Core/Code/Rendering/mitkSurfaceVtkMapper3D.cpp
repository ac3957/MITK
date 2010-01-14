/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#include "mitkSurfaceVtkMapper3D.h"
#include "mitkDataTreeNode.h"
#include "mitkProperties.h"
#include "mitkColorProperty.h"
#include "mitkLookupTableProperty.h"
#include "mitkMaterialProperty.h"
#include "mitkVtkRepresentationProperty.h"
#include "mitkVtkInterpolationProperty.h"
#include "mitkVtkScalarModeProperty.h"
#include "mitkClippingProperty.h"

#include "mitkShaderEnumProperty.h"
#include "mitkShaderRepository.h"


#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkPointData.h>
#include <vtkPlaneCollection.h>


const mitk::Surface* mitk::SurfaceVtkMapper3D::GetInput()
{
  return static_cast<const mitk::Surface * > ( GetData() );
}

mitk::SurfaceVtkMapper3D::SurfaceVtkMapper3D()
{
 // m_Prop3D = vtkActor::New();
  m_GenerateNormals = false;
}

mitk::SurfaceVtkMapper3D::~SurfaceVtkMapper3D()
{
 // m_Prop3D->Delete();                                  
}

void mitk::SurfaceVtkMapper3D::GenerateData(mitk::BaseRenderer* renderer)
{
  LocalStorage *ls = m_LSH.GetLocalStorage(renderer);
//  m_Prop3D = ls->m_Actor;
  
  bool visible = IsVisible(renderer);

  if(visible==false)
  {
    ls->m_Actor->VisibilityOff();
    return;
  }

  //
  // get the TimeSlicedGeometry of the input object
  //
  mitk::Surface::Pointer input  = const_cast< mitk::Surface* >( this->GetInput() );
  const TimeSlicedGeometry* inputTimeGeometry = input->GetTimeSlicedGeometry();
  if(( inputTimeGeometry == NULL ) || ( inputTimeGeometry->GetTimeSteps() == 0 ) )
  {
    ls->m_Actor->VisibilityOff();
    return;
  }

  //
  // get the world time
  //
  const Geometry2D* worldGeometry = renderer->GetCurrentWorldGeometry2D();
  assert( worldGeometry != NULL );
  ScalarType time = worldGeometry->GetTimeBounds()[ 0 ];

  //
  // convert the world time in time steps of the input object
  //
  int timestep=0;
  if( time > ScalarTypeNumericTraits::NonpositiveMin() )
    timestep = inputTimeGeometry->MSToTimeStep( time );
  if( inputTimeGeometry->IsValidTime( timestep ) == false )
  {
    ls->m_Actor->VisibilityOff();
    return;
  }
//  MITK_INFO << "time: "<< time << std::endl;
//  MITK_INFO << "timestep: "<<timestep << std::endl;
  
  //
  // set the input-object at time t for the mapper
  //

  vtkPolyData * polydata = input->GetVtkPolyData( timestep );
  if(polydata == NULL) 
  {
    ls->m_Actor->VisibilityOff();
    return;
  }

  if ( m_GenerateNormals )
  {
    ls->m_VtkPolyDataNormals->SetInput( polydata );
    ls->m_VtkPolyDataMapper->SetInput( ls->m_VtkPolyDataNormals->GetOutput() );
  }
  else
  {
    ls->m_VtkPolyDataMapper->SetInput( polydata );
  }

  //
  // apply properties read from the PropertyList
  //
  ApplyProperties(ls->m_Actor, renderer);

  if(visible)
    ls->m_Actor->VisibilityOn();
}


void mitk::SurfaceVtkMapper3D::ApplyProperties(vtkActor* /*actor*/, mitk::BaseRenderer* renderer)
{
  LocalStorage *ls = m_LSH.GetLocalStorage(renderer);
//  m_Prop3D = ls->m_Actor;
    
  mitk::MaterialProperty* materialProperty;
  
  //first check render specific property, then the regular one
  bool setMaterial = false;
  
  this->GetDataTreeNode()->GetProperty(materialProperty, "material", renderer);
  if ( materialProperty != NULL && this->GetDataTreeNode()->GetPropertyList(renderer)->IsEnabled("material"))
    setMaterial = true;
  else 
  {
    this->GetDataTreeNode()->GetProperty(materialProperty, "material");//without renderer?
    if (materialProperty != NULL && this->GetDataTreeNode()->GetPropertyList()->IsEnabled("material"))
      setMaterial = true;
  }
    
  if (setMaterial)
  {
    vtkProperty* property = ls->m_Actor->GetProperty();
    //property->SetColor( materialProperty->GetColor().GetDataPointer() );
    property->SetAmbientColor( materialProperty->GetColor().GetDataPointer() );    
    property->SetAmbient( materialProperty->GetColorCoefficient() );    
    property->SetDiffuseColor(materialProperty->GetColor().GetDataPointer() );    
    property->SetDiffuse( materialProperty->GetColorCoefficient() );
    property->SetSpecularColor( materialProperty->GetSpecularColor().GetDataPointer() );
    property->SetSpecular( materialProperty->GetSpecularCoefficient() );
    property->SetSpecularPower( materialProperty->GetSpecularPower() );
    property->SetOpacity( materialProperty->GetOpacity() );
    property->SetInterpolation( materialProperty->GetVtkInterpolation() );
    property->SetRepresentation( materialProperty->GetVtkRepresentation() );
    property->SetLineWidth( materialProperty->GetLineWidth() );
  }
  else
  {
    Superclass::ApplyProperties( ls->m_Actor, renderer ) ;
    //reset the default values in case no material is used
    vtkProperty* property = ls->m_Actor->GetProperty();

    property->SetAmbient( 0.0f );    
    property->SetDiffuse( 1.0f );
    property->SetSpecular( 0.0f );
    property->SetSpecularPower( 1.0f );

    float lineWidth = 1;
    this->GetDataTreeNode()->GetFloatProperty("wireframe line width", lineWidth);
    ls->m_Actor->GetProperty()->SetLineWidth( lineWidth );
  }

  mitk::ShaderRepository::GetGlobalShaderRepository()->ApplyProperties(this->GetDataTreeNode(),ls->m_Actor,renderer,ls->m_ShaderTimestampUpdate);

  mitk::LookupTableProperty::Pointer lookupTableProp;
  this->GetDataTreeNode()->GetProperty(lookupTableProp, "LookupTable", renderer);
  if (lookupTableProp.IsNotNull() )
  {
    ls->m_VtkPolyDataMapper->SetLookupTable(lookupTableProp->GetLookupTable()->GetVtkLookupTable());
  }

  mitk::LevelWindow levelWindow;
  if(this->GetDataTreeNode()->GetLevelWindow(levelWindow, renderer, "levelWindow"))
  {
    ls->m_VtkPolyDataMapper->SetScalarRange(levelWindow.GetLowerWindowBound(),levelWindow.GetUpperWindowBound());
  }
  else
  if(this->GetDataTreeNode()->GetLevelWindow(levelWindow, renderer))
  {
    ls->m_VtkPolyDataMapper->SetScalarRange(levelWindow.GetLowerWindowBound(),levelWindow.GetUpperWindowBound());
  }
  
  mitk::VtkRepresentationProperty* representationProperty;
  this->GetDataTreeNode()->GetProperty(representationProperty, "representation", renderer);
  if ( representationProperty != NULL )
    ls->m_Actor->GetProperty()->SetRepresentation( representationProperty->GetVtkRepresentation() );
  
  mitk::VtkInterpolationProperty* interpolationProperty;
  this->GetDataTreeNode()->GetProperty(interpolationProperty, "interpolation", renderer);
  if ( interpolationProperty != NULL )
    ls->m_Actor->GetProperty()->SetInterpolation( interpolationProperty->GetVtkInterpolation() );
  
  bool scalarVisibility = false;
  this->GetDataTreeNode()->GetBoolProperty("scalar visibility", scalarVisibility);
  ls->m_VtkPolyDataMapper->SetScalarVisibility( (scalarVisibility ? 1 : 0) );

  if(scalarVisibility)
  {
    mitk::VtkScalarModeProperty* scalarMode;
    if(this->GetDataTreeNode()->GetProperty(scalarMode, "scalar mode", renderer))
    {
      ls->m_VtkPolyDataMapper->SetScalarMode(scalarMode->GetVtkScalarMode());
    }
    else
      ls->m_VtkPolyDataMapper->SetScalarModeToDefault();

    bool colorMode = false;
    this->GetDataTreeNode()->GetBoolProperty("color mode", colorMode);
    ls->m_VtkPolyDataMapper->SetColorMode( (colorMode ? 1 : 0) );

    float scalarsMin = 0;
    if (dynamic_cast<mitk::FloatProperty *>(this->GetDataTreeNode()->GetProperty("ScalarsRangeMinimum")) != NULL)
      scalarsMin = dynamic_cast<mitk::FloatProperty*>(this->GetDataTreeNode()->GetProperty("ScalarsRangeMinimum"))->GetValue();

    float scalarsMax = 1.0;
    if (dynamic_cast<mitk::FloatProperty *>(this->GetDataTreeNode()->GetProperty("ScalarsRangeMaximum")) != NULL)
      scalarsMax = dynamic_cast<mitk::FloatProperty*>(this->GetDataTreeNode()->GetProperty("ScalarsRangeMaximum"))->GetValue();

    ls->m_VtkPolyDataMapper->SetScalarRange(scalarsMin,scalarsMax);
  }

  // deprecated settings
  bool deprecatedUseCellData = false;
  this->GetDataTreeNode()->GetBoolProperty("deprecated useCellDataForColouring", deprecatedUseCellData);

  bool deprecatedUsePointData = false;
  this->GetDataTreeNode()->GetBoolProperty("deprecated usePointDataForColouring", deprecatedUsePointData);
                  
  if (deprecatedUseCellData)
  {
    ls->m_VtkPolyDataMapper->SetColorModeToDefault();
    ls->m_VtkPolyDataMapper->SetScalarRange(0,255);
    ls->m_VtkPolyDataMapper->ScalarVisibilityOn();
    ls->m_VtkPolyDataMapper->SetScalarModeToUseCellData();
    ls->m_Actor->GetProperty()->SetSpecular (1);
    ls->m_Actor->GetProperty()->SetSpecularPower (50);
    ls->m_Actor->GetProperty()->SetInterpolationToPhong();
  }
  else if (deprecatedUsePointData)
  {
    float scalarsMin = 0;
    if (dynamic_cast<mitk::FloatProperty *>(this->GetDataTreeNode()->GetProperty("ScalarsRangeMinimum")) != NULL)
      scalarsMin = dynamic_cast<mitk::FloatProperty*>(this->GetDataTreeNode()->GetProperty("ScalarsRangeMinimum"))->GetValue();

    float scalarsMax = 0.1;
    if (dynamic_cast<mitk::FloatProperty *>(this->GetDataTreeNode()->GetProperty("ScalarsRangeMaximum")) != NULL)
      scalarsMax = dynamic_cast<mitk::FloatProperty*>(this->GetDataTreeNode()->GetProperty("ScalarsRangeMaximum"))->GetValue();

    ls->m_VtkPolyDataMapper->SetScalarRange(scalarsMin,scalarsMax);
    ls->m_VtkPolyDataMapper->SetColorModeToMapScalars();
    ls->m_VtkPolyDataMapper->ScalarVisibilityOn();
    ls->m_Actor->GetProperty()->SetSpecular (1);
    ls->m_Actor->GetProperty()->SetSpecularPower (50);
    ls->m_Actor->GetProperty()->SetInterpolationToPhong();
  }

  int deprecatedScalarMode = VTK_COLOR_MODE_DEFAULT;
  if(this->GetDataTreeNode()->GetIntProperty("deprecated scalar mode", deprecatedScalarMode, renderer))
  {
    ls->m_VtkPolyDataMapper->SetScalarMode(deprecatedScalarMode);
    ls->m_VtkPolyDataMapper->ScalarVisibilityOn();
    ls->m_Actor->GetProperty()->SetSpecular (1);
    ls->m_Actor->GetProperty()->SetSpecularPower (50);
    //m_Actor->GetProperty()->SetInterpolationToPhong();
  }


  // Check whether one or more ClippingProperty objects have been defined for
  // this node. Check both renderer specific and global property lists, since
  // properties in both should be considered.
  const PropertyList::PropertyMap *rendererProperties = this->GetDataTreeNode()->GetPropertyList( renderer )->GetMap();
  const PropertyList::PropertyMap *globalProperties = this->GetDataTreeNode()->GetPropertyList( NULL )->GetMap();

  // Add clipping planes (if any)
  ls->m_ClippingPlaneCollection->RemoveAllItems();
  
  PropertyList::PropertyMap::const_iterator it;
  for ( it = rendererProperties->begin(); it != rendererProperties->end(); ++it )
  {
    this->CheckForClippingProperty( renderer,(*it).second.first.GetPointer() );
  }

  for ( it = globalProperties->begin(); it != globalProperties->end(); ++it )
  {
    this->CheckForClippingProperty( renderer,(*it).second.first.GetPointer() );
  }

  if ( ls->m_ClippingPlaneCollection->GetNumberOfItems() > 0 )
  {
    ls->m_VtkPolyDataMapper->SetClippingPlanes( ls->m_ClippingPlaneCollection );
  }
  else
  {
    ls->m_VtkPolyDataMapper->RemoveAllClippingPlanes();
  }
  
  
}

vtkProp *mitk::SurfaceVtkMapper3D::GetVtkProp(mitk::BaseRenderer *renderer)
{
  LocalStorage *ls = m_LSH.GetLocalStorage(renderer);
  return ls->m_Actor;
}

void mitk::SurfaceVtkMapper3D::CheckForClippingProperty( mitk::BaseRenderer* renderer, mitk::BaseProperty *property )
{
  LocalStorage *ls = m_LSH.GetLocalStorage(renderer);
//  m_Prop3D = ls->m_Actor;

  ClippingProperty *clippingProperty = dynamic_cast< ClippingProperty * >( property );

  if ( (clippingProperty != NULL)
    && (clippingProperty->GetClippingEnabled()) )
  {
    const Point3D &origin = clippingProperty->GetOrigin();
    const Vector3D &normal = clippingProperty->GetNormal();

    vtkPlane *clippingPlane = vtkPlane::New();
    clippingPlane->SetOrigin( origin[0], origin[1], origin[2] );
    clippingPlane->SetNormal( normal[0], normal[1], normal[2] );

    ls->m_ClippingPlaneCollection->AddItem( clippingPlane );

    clippingPlane->UnRegister( NULL );
  }
}


void mitk::SurfaceVtkMapper3D::SetDefaultProperties(mitk::DataTreeNode* node, mitk::BaseRenderer* renderer, bool overwrite)
{
  mitk::ShaderRepository::GetGlobalShaderRepository()->AddDefaultProperties(node,renderer,overwrite);

  node->AddProperty( "wireframe line width", mitk::FloatProperty::New(1.0), renderer, overwrite );
  node->AddProperty( "material", mitk::MaterialProperty::New( 1.0, 1.0, 1.0, 1.0, node ), renderer, overwrite );
  node->AddProperty( "scalar visibility", mitk::BoolProperty::New(false), renderer, overwrite );
  node->AddProperty( "color mode", mitk::BoolProperty::New(false), renderer, overwrite );
  node->AddProperty( "representation", mitk::VtkRepresentationProperty::New(), renderer, overwrite );
  node->AddProperty( "interpolation", mitk::VtkInterpolationProperty::New(), renderer, overwrite );
  node->AddProperty( "scalar mode", mitk::VtkScalarModeProperty::New(), renderer, overwrite );
  mitk::Surface::Pointer surface = dynamic_cast<Surface*>(node->GetData());
  if(surface.IsNotNull())
  {
    if((surface->GetVtkPolyData() != 0) && (surface->GetVtkPolyData()->GetPointData() != NULL) && (surface->GetVtkPolyData()->GetPointData()->GetScalars() != 0))
    {
      node->AddProperty( "scalar visibility", mitk::BoolProperty::New(true), renderer, overwrite );
      node->AddProperty( "color mode", mitk::BoolProperty::New(true), renderer, overwrite );
    }
  }
  Superclass::SetDefaultProperties(node, renderer, overwrite);
}

void mitk::SurfaceVtkMapper3D::SetImmediateModeRenderingOn(int  /*on*/)
{
/*
  if (m_VtkPolyDataMapper != NULL) 
    m_VtkPolyDataMapper->SetImmediateModeRendering(on);
*/
}
