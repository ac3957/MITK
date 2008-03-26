/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision: $

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#include "mitkUnstructuredGridVtkMapper3D.h"
#include "mitkDataTreeNode.h"
#include "mitkProperties.h"
#include "mitkTransferFunctionProperty.h"
#include "mitkColorProperty.h"
//#include "mitkLookupTableProperty.h"
#include "mitkMaterialProperty.h"
#include "mitkGridRepresentationProperty.h"
#include "mitkGridVolumeMapperProperty.h"
#include "mitkVtkInterpolationProperty.h"
#include "mitkVtkScalarModeProperty.h"

#include "mitkDataStorage.h"

#include <vtkUnstructuredGrid.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkProperty.h>

#include <vtkPlanes.h>



const mitk::UnstructuredGrid* mitk::UnstructuredGridVtkMapper3D::GetInput()
{
  return static_cast<const mitk::UnstructuredGrid * > ( GetData() );
}


mitk::UnstructuredGridVtkMapper3D::UnstructuredGridVtkMapper3D()
{
  
  m_VtkTriangleFilter = vtkDataSetTriangleFilter::New();
   
  m_Assembly = vtkAssembly::New();
  
  m_Volume = vtkVolume::New();
  m_Actor = vtkActor::New();
  m_ActorWireframe = vtkActor::New();
  
  m_VtkDataSetMapper = vtkUnstructuredGridMapper::New();
  m_VtkDataSetMapper->SetResolveCoincidentTopologyToPolygonOffset();
  m_VtkDataSetMapper->SetResolveCoincidentTopologyPolygonOffsetParameters(0,1);
  m_Actor->SetMapper(m_VtkDataSetMapper);
  
  m_VtkDataSetMapper2 = vtkUnstructuredGridMapper::New();
  m_VtkDataSetMapper2->SetResolveCoincidentTopologyToPolygonOffset();
  m_VtkDataSetMapper2->SetResolveCoincidentTopologyPolygonOffsetParameters(1,1);
  m_ActorWireframe->SetMapper(m_VtkDataSetMapper2);
  m_ActorWireframe->GetProperty()->SetRepresentationToWireframe();

  m_Assembly->AddPart(m_Actor);
  m_Assembly->AddPart(m_ActorWireframe);
  m_Assembly->AddPart(m_Volume);
  m_Prop3D = m_Assembly;
  
  m_VtkVolumeRayCastMapper = 0;
  #if (VTK_MAJOR_VERSION >= 5)
  m_VtkPTMapper = 0;
  m_VtkVolumeZSweepMapper = 0;
  #endif
  
  //m_GenerateNormals = false;
}


mitk::UnstructuredGridVtkMapper3D::~UnstructuredGridVtkMapper3D()
{

  if (m_VtkTriangleFilter != 0)
    m_VtkTriangleFilter->Delete();   

  if (m_VtkVolumeRayCastMapper != 0)
    m_VtkVolumeRayCastMapper->Delete();
  
  #if (VTK_MAJOR_VERSION >= 5)
  if (m_VtkVolumeZSweepMapper != 0)
    m_VtkVolumeZSweepMapper->Delete();
    
  if (m_VtkPTMapper != 0)
    m_VtkPTMapper->Delete();
  #endif
    
  if (m_VtkDataSetMapper != 0)
    m_VtkDataSetMapper->Delete();
  
  if (m_VtkDataSetMapper2 != 0)
      m_VtkDataSetMapper2->Delete();
  
  if(m_Prop3D != m_Assembly)
    m_Assembly->Delete();
  
  if (m_Actor != 0)
    m_Actor->Delete();
  
  if (m_ActorWireframe != 0)
      m_ActorWireframe->Delete();
     
  if (m_Volume != 0)
    m_Volume->Delete();
  
}

void mitk::UnstructuredGridVtkMapper3D::GenerateData()
{
  m_Assembly->VisibilityOn();
  
  m_ActorWireframe->GetProperty()->SetAmbient(1.0);
  m_ActorWireframe->GetProperty()->SetDiffuse(0.0);
  m_ActorWireframe->GetProperty()->SetSpecular(0.0);
}

void mitk::UnstructuredGridVtkMapper3D::GenerateData(mitk::BaseRenderer* renderer)
{
  
  if(!IsVisible(renderer))
  {
    m_Assembly->VisibilityOff();
    return;
  }

  //
  // get the TimeSlicedGeometry of the input object
  //
  mitk::UnstructuredGrid::Pointer input  = const_cast< mitk::UnstructuredGrid* >( this->GetInput() );
  const TimeSlicedGeometry* inputTimeGeometry = input->GetTimeSlicedGeometry();
  if(( inputTimeGeometry == 0 ) || ( inputTimeGeometry->GetTimeSteps() == 0 ) )
  {
    m_Assembly->VisibilityOff();
    return;
  }

  int timestep = this->GetTimestep();
  if( inputTimeGeometry->IsValidTime( timestep ) == false )
  {
    m_Assembly->VisibilityOff();
    return;
  }

  //
  // set the input-object at time t for the mapper
  //
  vtkUnstructuredGrid * grid = input->GetVtkUnstructuredGrid( timestep );
  if(grid == 0) 
  {
    m_Assembly->VisibilityOff();
    return;
  }

  m_VtkTriangleFilter->SetInput(grid);  
  m_VtkDataSetMapper->SetInput(grid);
  m_VtkDataSetMapper2->SetInput(grid);
  
  mitk::DataTreeNode::ConstPointer node = this->GetDataTreeNode();
  bool clip = false;
  node->GetBoolProperty("enable clipping", clip);
  mitk::DataTreeNode::Pointer bbNode = mitk::DataStorage::GetInstance()->GetNamedDerivedNode("Clipping Bounding Object", node); 
  if (clip && bbNode.IsNotNull())
  {
    m_VtkDataSetMapper->SetBoundingObject(dynamic_cast<mitk::BoundingObject*>(bbNode->GetData()));
    m_VtkDataSetMapper2->SetBoundingObject(dynamic_cast<mitk::BoundingObject*>(bbNode->GetData()));
  }
  else
  {
    m_VtkDataSetMapper->SetBoundingObject(0);
    m_VtkDataSetMapper2->SetBoundingObject(0);
  }
  
  //
  // apply properties read from the PropertyList
  //
  SetProperties(renderer);
}

void mitk::UnstructuredGridVtkMapper3D::SetProperties(mitk::BaseRenderer* renderer)
{
  mitk::DataTreeNode::Pointer node = this->GetDataTreeNode();
  vtkVolumeProperty* volProp = m_Volume->GetProperty();
  vtkProperty* property = m_Actor->GetProperty();


  mitk::MaterialProperty* materialProperty;
  this->GetDataTreeNode()->GetProperty(materialProperty, "material");
  if ( materialProperty != 0 )
  {
    if ( materialProperty->GetRenderer() == 0 || materialProperty->GetRenderer() == renderer )
    {
      property->SetColor( materialProperty->GetColor().GetDataPointer() );
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
      
      m_ActorWireframe->GetProperty()->SetOpacity(materialProperty->GetOpacity());
      m_ActorWireframe->GetProperty()->SetColor( materialProperty->GetColor().GetDataPointer() );
    }
  }
  
  mitk::TransferFunctionProperty::Pointer transferFuncProp;
  if (node->GetProperty(transferFuncProp, "TransferFunction", renderer))
  {
    mitk::TransferFunction::Pointer transferFunction = transferFuncProp->GetValue();
    
    volProp->SetColor(transferFunction->GetColorTransferFunction());
    volProp->SetScalarOpacity(transferFunction->GetScalarOpacityFunction());
    
    m_VtkDataSetMapper->SetLookupTable(transferFunction->GetColorTransferFunction());
    m_VtkDataSetMapper2->SetLookupTable(transferFunction->GetColorTransferFunction());
  }
  
  mitk::GridRepresentationProperty::Pointer gridRepProp;
  if (node->GetProperty(gridRepProp, "grid representation", renderer))
  {
    mitk::GridRepresentationProperty::IdType type = gridRepProp->GetValueAsId();
    bool isVolume = false;
    switch (type) {
      case mitk::GridRepresentationProperty::POINTS:
        property->SetRepresentationToPoints();
        break;
      case mitk::GridRepresentationProperty::WIREFRAME:
        property->SetRepresentationToWireframe();
        break;
      case mitk::GridRepresentationProperty::SURFACE:
        property->SetRepresentationToSurface();
        break;
      case mitk::GridRepresentationProperty::VOLUME:
        m_Assembly->RemovePart(m_Actor);
        m_Assembly->RemovePart(m_ActorWireframe);
        m_Assembly->AddPart(m_Volume);
        isVolume = true;
        break;
    }
    
    if (!isVolume) {
      m_Assembly->RemovePart(m_Volume);
      m_Assembly->AddPart(m_Actor);
      if (type == mitk::GridRepresentationProperty::SURFACE)
        m_Assembly->AddPart(m_ActorWireframe);
      else
        m_Assembly->RemovePart(m_ActorWireframe);
    }
    
    
  }
  
  mitk::GridVolumeMapperProperty::Pointer mapperProp;
  if (node->GetProperty(mapperProp, "grid volume mapper", renderer))
  {
    mitk::GridVolumeMapperProperty::IdType type = mapperProp->GetValueAsId();
    switch (type) {
      case mitk::GridVolumeMapperProperty::RAYCAST:
        if (m_VtkVolumeRayCastMapper == 0) {
          m_VtkVolumeRayCastMapper = vtkUnstructuredGridVolumeRayCastMapper::New();
          m_VtkVolumeRayCastMapper->SetInput(m_VtkTriangleFilter->GetOutput());
        }
        m_Volume->SetMapper(m_VtkVolumeRayCastMapper);
        break;
      #if (VTK_MAJOR_VERSION >= 5)
      case mitk::GridVolumeMapperProperty::PT:
        if (m_VtkPTMapper == 0) {
          m_VtkPTMapper = vtkProjectedTetrahedraMapper::New();
          m_VtkPTMapper->SetInputConnection(m_VtkTriangleFilter->GetOutputPort());
        }
        m_Volume->SetMapper(m_VtkPTMapper);
        break;
      case mitk::GridVolumeMapperProperty::ZSWEEP:
        if (m_VtkVolumeZSweepMapper == 0) {
          m_VtkVolumeZSweepMapper = vtkUnstructuredGridVolumeZSweepMapper::New();
          m_VtkVolumeZSweepMapper->SetInputConnection(m_VtkTriangleFilter->GetOutputPort());
        }
        m_Volume->SetMapper(m_VtkVolumeZSweepMapper);
        break;
      #endif
    }
  }
    

//   mitk::LevelWindow levelWindow;
//   if(node->GetLevelWindow(levelWindow, renderer, "levelWindow"))
//   {
//     m_VtkVolumeRayCastMapper->SetScalarRange(levelWindow.GetMin(),levelWindow.GetMax());
//   }
//   else
//   if(node->GetLevelWindow(levelWindow, renderer))
//   {
//     m_VtkVolumeRayCastMapper->SetScalarRange(levelWindow.GetMin(),levelWindow.GetMax());
//   }
//   
//   mitk::VtkRepresentationProperty* representationProperty;
//   node->GetProperty(representationProperty, "representation", renderer);
//   if ( representationProperty != NULL )
//     m_Volume->GetProperty()->SetRepresentation( representationProperty->GetVtkRepresentation() );
//   
//   mitk::VtkInterpolationProperty* interpolationProperty;
//   node->GetProperty(interpolationProperty, "interpolation", renderer);
//   if ( interpolationProperty != NULL )
//     m_Volume->GetProperty()->SetInterpolation( interpolationProperty->GetVtkInterpolation() );
//   
//   bool scalarVisibility = false;
//   node->GetBoolProperty("scalar visibility", scalarVisibility);
//   m_VtkVolumeRayCastMapper->SetScalarVisibility( (scalarVisibility ? 1 : 0) );
// 
//   if(scalarVisibility)
//   {
//     mitk::VtkScalarModeProperty* scalarMode;
//     if(node->GetProperty(scalarMode, "scalar mode", renderer))
//     {
//       m_VtkVolumeRayCastMapper->SetScalarMode(scalarMode->GetVtkScalarMode());
//     }
//     else
//       m_VtkVolumeRayCastMapper->SetScalarModeToDefault();
// 
//     bool colorMode = false;
//     node->GetBoolProperty("color mode", colorMode);
//     m_VtkVolumeRayCastMapper->SetColorMode( (colorMode ? 1 : 0) );
// 
//     float scalarsMin = 0;
//     if (dynamic_cast<mitk::FloatProperty *>(node->GetProperty("ScalarsRangeMinimum").GetPointer()) != NULL)
//       scalarsMin = dynamic_cast<mitk::FloatProperty*>(node->GetProperty("ScalarsRangeMinimum").GetPointer())->GetValue();
// 
//     float scalarsMax = 1.0;
//     if (dynamic_cast<mitk::FloatProperty *>(node->GetProperty("ScalarsRangeMaximum").GetPointer()) != NULL)
//       scalarsMax = dynamic_cast<mitk::FloatProperty*>(node->GetProperty("ScalarsRangeMaximum").GetPointer())->GetValue();
// 
//     m_VtkVolumeRayCastMapper->SetScalarRange(scalarsMin,scalarsMax);
//   }

}

