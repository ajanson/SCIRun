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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Core/Datatypes/Tests/MatrixTestCases.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/MatrixIO.h>
#include <Core/Datatypes/MatrixComparison.h>
#include <Core/Algorithms/DataIO/ReadMatrix.h>
#include <boost/filesystem.hpp>

using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Core::Algorithms::DataIO;
using namespace TestUtils;

TEST(ReadMatrixTests, CanReadFromStream)
{
  const std::string matrixString = "0 1 2\n3 4 5\n6 7 8\n";

  std::istringstream istr(matrixString);

  DenseMatrix in;

  istr >> in;

  EXPECT_EQ(matrix1(), in);
}

TEST(ReadMatrixTests, RoundTripViaString)
{
  DenseMatrix m = matrix1();

  std::ostringstream ostr;
  ostr << m;
  std::string str = ostr.str();
  std::istringstream istr(str);
  DenseMatrix m2;
  istr >> m2;

  EXPECT_EQ(m, m2);
}

TEST(ReadMatrixAlgorithmTest, TestFromRealTextFile)
{
  ReadMatrixAlgorithm algo;
  const std::string filename = "E:\\git\\SCIRunGUIPrototype\\src\\Samples\\matrix1.txt";
  if (boost::filesystem::exists(filename))
  {
    DenseMatrixConstHandle matrix = algo.run(filename);

    EXPECT_EQ(matrixNonSquare(), *matrix);
  }
  else
    std::cout << "file does not exist, skipping test." << std::endl;
}