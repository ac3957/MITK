/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


#ifndef ShImageSource_H_HEADER_INCLUDED_C1E7D6EC
#define ShImageSource_H_HEADER_INCLUDED_C1E7D6EC

#include "mitkImageSource.h"
#include "mitkShImage.h"
#include <MitkDiffusionCoreExports.h>

namespace mitk {

class MITKDIFFUSIONCORE_EXPORT ShImageSource : public ImageSource
{
public:

  typedef mitk::ShImage OutputImageType;
  typedef OutputImageType::Pointer OutputImagePointer;
  typedef SlicedData::RegionType OutputImageRegionType;
  typedef itk::DataObject::Pointer DataObjectPointer;

  mitkClassMacro(ShImageSource,ImageSource);
  itkFactorylessNewMacro(Self)
  itkCloneMacro(Self)

  /**
   * Allocates a new output object and returns it. Currently the
   * index idx is not evaluated.
   * @param idx the index of the output for which an object should be created
   * @returns the new object
   */
  itk::DataObject::Pointer MakeOutput ( DataObjectPointerArraySizeType idx ) override;

  /**
   * This is a default implementation to make sure we have something.
   * Once all the subclasses of ProcessObject provide an appopriate
   * MakeOutput(), then ProcessObject::MakeOutput() can be made pure
   * virtual.
   */
  itk::DataObject::Pointer MakeOutput(const DataObjectIdentifierType &name) override;

protected:
  ShImageSource();
  ~ShImageSource() override {}

};

} // namespace mitk

#endif /* ShImageSource_H_HEADER_INCLUDED_C1E7D6EC */
