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

#ifndef mitkThresholdComponentsTool_h_Included
#define mitkThresholdComponentsTool_h_Included

#include "mitkAutoSegmentationTool.h"
#include "mitkCommon.h"
#include "mitkDataStorage.h"
#include "mitkPointSet.h"
#include "mitkPointSetDataInteractor.h"
#include <MitkSegmentationExports.h>

namespace us
{
  class ModuleResource;
}

namespace mitk
{
  /**
  \brief Dummy Tool for ThresholdComponentsToolGUI to get Tool functionality for ThresholdComponents.
  The actual logic is implemented in QmitkThresholdComponentsToolGUI.

  \ingroup ToolManagerEtAl
  \sa mitk::Tool
  \sa QmitkInteractiveSegmentation

  */
  class MITKSEGMENTATION_EXPORT ThresholdComponentsTool : public AutoSegmentationTool
  {
  public:
    /**
     * @brief mitkClassMacro
     */
    mitkClassMacro(ThresholdComponentsTool, AutoSegmentationTool);
    itkFactorylessNewMacro(Self) itkCloneMacro(Self)

      bool CanHandle(BaseData *referenceData) const override;

    /**
     * @brief Get XPM
     * @return NULL
     */
    virtual const char **GetXPM() const override;

    /**
     * @brief Get name
     * @return name of the Tool
     */
    virtual const char *GetName() const override;

    /**
     * @brief Get icon resource
     * @return the resource Object of the Icon
     */
    us::ModuleResource GetIconResource() const override;

    /**
     * @brief Adds interactor for the seedpoint and creates a seedpoint if neccessary.
     *
     *
     */
    virtual void Activated() override;

    /**
     * @brief Removes all set points and interactors.
     *
     *
     */
    virtual void Deactivated() override;

    /**
     * @brief get pointset node
     * @return the point set node
     */
    virtual DataNode::Pointer GetPointSetNode();

    /**
     * @brief get reference data
     * @return the current reference data.
     */
    mitk::DataNode *GetReferenceData();

    /**
     * @brief Get working data
     * @return a list of all working data.
     */
    mitk::DataNode *GetWorkingData();

    /**
     * @brief Get datastorage
     * @return the current data storage.
     */
    mitk::DataStorage *GetDataStorage();

    void ConfirmSegmentation();
    
    PointSet::Pointer m_PointSet;
    PointSetDataInteractor::Pointer m_SeedPointInteractor;
    DataNode::Pointer m_PointSetNode;
  protected:
    /**
     * @brief constructor
     */
    ThresholdComponentsTool(); // purposely hidden

    /**
     * @brief destructor
     */
    virtual ~ThresholdComponentsTool();

//  private:

  };

} // namespace

#endif
