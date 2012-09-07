/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
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

#include <iostream>
#include <Modules/Visualization/MatrixAsVectorField.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/Geometry.h>

using namespace SCIRun::Modules::Visualization;
using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Dataflow::Networks;

MatrixAsVectorFieldModule::MatrixAsVectorFieldModule() : Module(ModuleLookupInfo("MatrixAsVectorFieldModule", "Visualization", "SCIRun")) {}

void MatrixAsVectorFieldModule::execute()
{
  DatatypeHandleOption input = get_input_handle(0);
  if (!input)
    throw std::logic_error("TODO Input data required, need to move this check to Module base class!");

  DenseMatrixHandle mat = boost::dynamic_pointer_cast<DenseMatrix>(*input); //TODO : clean
  if (!mat)
  {
    std::cout << "Matrix was null, returning." << std::endl;
    //TODO log error? send null? check standard practice.
    return;
  }

  if (mat->rows() != 6)
  {
    std::cout << "Not outputting, matrix does not have 6 rows." << std::endl;
    return;
  }

  std::cout << "Matrix value received." << std::endl;

  GeometryHandle geom(new GeometryObject(mat));
  send_output_handle(0, geom);
}