
/*
   For more information, please see: http://software.sci.utah.edu
   The MIT License
   Copyright (c) 2015 Scientific Computing and Imaging Institute,
   University of Utah.
   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <Modules/BrainStimulator/ModelTMSCoil.h>
#include <Interface/Modules/BrainStimulator/ModelTMSCoilDialog.h>
#include <Core/Algorithms/BrainStimulator/ModelGenericCoilAlgorithm.h>
#include <Dataflow/Network/ModuleStateInterface.h>

using namespace SCIRun::Gui;
using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Algorithms::BrainStimulator;

ModelTMSCoilDialog::ModelTMSCoilDialog(const std::string& name, ModuleStateHandle state,
  QWidget* parent /* = 0 */)
  : ModuleDialogGeneric(state, parent)
{
  setupUi(this);
  setWindowTitle(QString::fromStdString(name));
  fixSize();
  
  addComboBoxManager(Type_,Parameters::Type);
  addRadioButtonGroupManager({ CircularRadioButton_, Fig8RadioButton_}, Parameters::FigureOf8CoilShape);
  addDoubleSpinBoxManager(Current_, Parameters::Current);
  addDoubleSpinBoxManager(WingsAngle_, Parameters::WingsAngle);
  addDoubleSpinBoxManager(InnerRadius_, Parameters::InnerRadius);
  addDoubleSpinBoxManager(OuterRadius_, Parameters::OuterRadius);
  addDoubleSpinBoxManager(Distance_, Parameters::Distance);
  addDoubleSpinBoxManager(LayerStepSize_, Parameters::LayerStepSize);
  addSpinBoxManager(Rings_, Parameters::Rings); 
  addSpinBoxManager(Layers_, Parameters::Layers);
  addSpinBoxManager(LevelOfDetail_, Parameters::LevelOfDetail);
}

