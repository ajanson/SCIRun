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

#include <Core/Algorithms/Field/InterfaceWithCleaver2Algorithm.h>
#include <Core/Algorithms/Field/InterfaceWithCleaverAlgorithm.h>
#include <Core/Algorithms/Base/AlgorithmVariableNames.h>

//#include <cleaver/FloatField.h>
#include <cleaver2/vec3.h>
#include <cleaver2/BoundingBox.h>
#include <cleaver2/Cleaver.h>
#include <cleaver2/CleaverMesher.h>
#include <cleaver2/InverseField.h>
#include <cleaver2/SizingFieldCreator.h>
//#include <cleaver/PaddedVolume.h>
#include <cleaver2/Volume.h>
#include <cleaver2/TetMesh.h>
#include <Core/Algorithms/Base/AlgorithmPreconditions.h>

#include <Core/GeometryPrimitives/Vector.h>
#include <Core/Datatypes/Legacy/Field/Field.h>
#include <Core/Datatypes/Legacy/Field/VField.h>
#include <Core/Datatypes/Legacy/Field/VMesh.h>
#include <Core/Datatypes/Legacy/Field/FieldInformation.h>
#include <iostream>
#include <Core/GeometryPrimitives/Point.h>
#include <Core/Utils/StringUtil.h>
#include <boost/scoped_ptr.hpp>
#include <Core/Logging/Log.h>
#include <Core/Math/MiscMath.h>

using namespace SCIRun;
using namespace SCIRun::Core;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Algorithms::Fields;
using namespace SCIRun::Core::Geometry;

ALGORITHM_PARAMETER_DEF(Fields, Verbose);
ALGORITHM_PARAMETER_DEF(Fields, VolumeScaling);
ALGORITHM_PARAMETER_DEF(Fields, Lipschitz);
ALGORITHM_PARAMETER_DEF(Fields, Padding);

namespace detail
{
  using CleaverInputFieldList = std::vector<boost::shared_ptr<cleaver2::AbstractScalarField>>;

  static const double kDefaultAlpha = 0.4;
  static const double kDefaultAlphaLong = 0.357;
  static const double kDefaultAlphaShort = 0.203;
  static const double kDefaultScale = 1.0;
  static const double kDefaultLipschitz = 0.2;
  static const double kDefaultMultiplier = 1.0;
  static const int    kDefaultPadding = 0;
  static const int    kDefaultMaxIterations = 1000;
  static const double kDefaultSigma = 1.;

  struct Cleaver2Parameters
  {
    cleaver2::MeshType mesh_mode;
    double alphaLong;
    double alphaShort;
    double scaling;
    double lipschitz;
    double multiplier;
    bool verbose;
  };

  class Cleaver2Impl
  {
  public:
    Cleaver2Impl(const AlgorithmBase* algo, const Cleaver2Parameters& params) : algo_(algo), params_(params) {}

    FieldHandle doit(CleaverInputFieldList fields)
    {
      volume_.reset(new cleaver2::Volume(toVectorOfRawPointers(fields)));


      // TODO DAN: add optional padding to sizing field...
      /// Padding is now optional!
      //boost::shared_ptr<cleaver2::AbstractVolume> paddedVolume(volume);
      // const bool verbose = get(Verbose).toBool();
      // const bool pad = get(Padding).toBool();
      //
      // if (pad)
      // {
      //   paddedVolume.reset(new Cleaver::PaddedVolume(volume.get()));
      // }
      //
      // if (verbose)
      // {
      //  std::cout << "Input Dimensions: " << dims[0] << " x " << dims[1] << " x " << dims[2] << std::endl;
      //  if (pad)
      //   std::cout << "Padded Mesh with Volume Size " << paddedVolume->size().toString() << std::endl;
      //      else
      //         std::cout << "Creating Mesh with Volume Size " << volume->size().toString() << std::endl;
      // }

      addSizingFieldToVolume();

      bool verbose = SCIRun::Core::Logging::GeneralLog::Instance().verbose();  //TODO: expose

      bool simple = false; //TODO expose
      cleaver2::CleaverMesher mesher(simple);
      mesher.setVolume(volume_.get());
      const double alpha = kDefaultAlpha; //no expose
      mesher.setAlphaInit(alpha);

      //TODO: if fixed grid is checked, expose alpha short and long--see CLI line 454

      //-----------------------------------
      // Load background mesh if provided--TODO: copy from CLI:400
      //-----------------------------------
      cleaver2::TetMesh *bgMesh = nullptr;
      switch (params_.mesh_mode)
      {
        case cleaver2::Regular:
        {
          double alpha_long = kDefaultAlphaLong;
          double alpha_short = kDefaultAlphaShort;
          mesher.setAlphas(alpha_long, alpha_short);
          mesher.setRegular(true);
          bgMesh = mesher.createBackgroundMesh(verbose);
          break;
        }
        case cleaver2::Structured:
        {
          mesher.setRegular(false);
          bgMesh = mesher.createBackgroundMesh(verbose);
          break;
        }
      }

      //TODO--add output ports for background (bgMesh) and sizing field

      mesher.buildAdjacency(verbose);
      mesher.sampleVolume(verbose);
      mesher.computeAlphas(verbose);
      mesher.computeInterfaces(verbose);
      mesher.generalizeTets(verbose);
      mesher.snapsAndWarp(verbose);
      mesher.stencilTets(verbose);

      auto mesh(boost::shared_ptr<cleaver2::TetMesh>(mesher.getTetMesh()));

      //-----------------------------------------------------------
      // Fix jacobians if requested.--TODO expose: [x] reverse Jacobian
      //-----------------------------------------------------------
      bool fix_tets = true;
      if (fix_tets)
        mesh->fixVertexWindup(verbose);

      return convertCleaverOutputToField(mesh);
    }

    //TODO dan: need run-time check for float or double field data
    static boost::shared_ptr<cleaver2::AbstractScalarField> makeCleaver2FieldFromLatVol(FieldHandle field)
    {
      auto vmesh = field->vmesh();
      auto vfield = field->vfield();
      VMesh::dimension_type dims;
      vmesh->get_dimensions(dims);

      auto ptr = static_cast<float*>(vfield->fdata_pointer());

      auto cleaverField = boost::make_shared<cleaver2::ScalarField<float>>(ptr, dims[0], dims[1], dims[2]);
      cleaver2::BoundingBox bb(cleaver2::vec3::zero, cleaver2::vec3(dims[0], dims[1], dims[2]));
      cleaverField->setBounds(bb);
      const auto& transform = vmesh->get_transform();

      int x_spacing = fabs(transform.get_mat_val(0, 0)), y_spacing = fabs(transform.get_mat_val(1, 1)), z_spacing = fabs(transform.get_mat_val(2, 2));

      if (IsNan(x_spacing) || x_spacing <= 0) x_spacing = 1; /// dont allow negative or zero scaling of the bounding box
      if (IsNan(y_spacing) || y_spacing <= 0) y_spacing = 1;
      if (IsNan(z_spacing) || z_spacing <= 0) z_spacing = 1;

      cleaverField->setScale(cleaver2::vec3(x_spacing, y_spacing, z_spacing));

      return cleaverField;
    }

    void addSizingFieldToVolume()
    {
#if 0 //TODO: add optional sizing field port--convert from latvol
      if (have_sizing_field)
      {
        std::cout << "Loading sizing field: " << sizing_field << std::endl;
        std::vector<std::string> tmp(1,sizing_field);
        sizingField = NRRDTools::loadNRRDFiles(tmp);
        // todo(jon): add error handling
      }
      else
#endif
      {
       //TODO: expose all these in GUI--these are the defaults from CLI
        sizingField_.reset(cleaver2::SizingFieldCreator::createSizingFieldFromVolume(
          volume_.get(),
          (float)(1.0 / params_.lipschitz),
          (float)params_.scaling,
          (float)params_.multiplier,
          0, // padding--off
          (params_.mesh_mode != cleaver2::Regular),
          params_.verbose));
      }
      volume_->setSizingField(sizingField_.get());
    }

    FieldHandle convertCleaverOutputToField(boost::shared_ptr<cleaver2::TetMesh> mesh)
    {
      auto nr_of_tets = mesh->tets.size();
      auto nr_of_verts = mesh->verts.size();

      if (nr_of_tets == 0 || nr_of_verts == 0)
      {
        THROW_ALGORITHM_INPUT_ERROR_WITH(algo_, " Number of resulting tetrahedral nodes or elements is 0. If you disabled padding enable it and execute again. ");
      }

      //TODO: extract cleaverMesh->fld code, write inverse function for sizing field input mesh

      FieldInformation fi("TetVolMesh", 0, "double");   ///create output field

      auto output = CreateField(fi);
      auto omesh = output->vmesh();
      auto ofield = output->vfield();

      omesh->node_reserve(nr_of_verts);
      omesh->elem_reserve(nr_of_tets);

      for (auto i = 0; i < nr_of_verts; i++)
      {
        omesh->add_point(Point(mesh->verts[i]->pos().x, mesh->verts[i]->pos().y, mesh->verts[i]->pos().z));
      }

      VMesh::Node::array_type vdata;
      vdata.resize(4);
      std::vector<double> values(nr_of_tets);

      for (auto i = 0; i < nr_of_tets; i++)
      {
        vdata[0] = mesh->tets[i]->verts[0]->tm_v_index;
        vdata[1] = mesh->tets[i]->verts[1]->tm_v_index;
        vdata[2] = mesh->tets[i]->verts[2]->tm_v_index;
        vdata[3] = mesh->tets[i]->verts[3]->tm_v_index;
        omesh->add_elem(vdata);
        auto mat_label = mesh->tets[i]->mat_label;
        values[i] = mat_label;
      }
      ofield->resize_values();
      ofield->set_values(values);
      mesh->computeAngles();

      std::ostringstream ostr1, ostr2;
      ostr1 << "(nodes, elements, dims) - (" << nr_of_verts << " , " << nr_of_tets << " , " << volume_->size().toString() << ")" << std::endl;
      ostr2 << "(min angle, max angle) - (" << mesh->min_angle << " , " << mesh->max_angle << ")" << std::endl;

      algo_->remark(ostr1.str());
      algo_->remark(ostr2.str());
      return output;
    }
  private:
    const AlgorithmBase* algo_;
    Cleaver2Parameters params_;
    boost::shared_ptr<cleaver2::ScalarField<float>> sizingField_;
    boost::shared_ptr<cleaver2::Volume> volume_;
  };
}

InterfaceWithCleaver2Algorithm::InterfaceWithCleaver2Algorithm()
{
  addParameter(Parameters::Verbose, true);
  // addParameter(Padding,true);
  // addOption(VolumeScalingOption, "Relative size", "Absolute size|Relative size|None");
  addParameter(Parameters::VolumeScaling, detail::kDefaultScale);
  addParameter(Parameters::Lipschitz, detail::kDefaultLipschitz);
  addParameter(Parameters::Padding, detail::kDefaultPadding);
}

FieldHandle InterfaceWithCleaver2Algorithm::runImpl(const FieldList& input) const
{
  std::vector<FieldHandle> inputs;
  std::copy_if(input.begin(), input.end(), std::back_inserter(inputs), [](FieldHandle f) { return f; });

  if (inputs.empty())
  {
    THROW_ALGORITHM_INPUT_ERROR(" No input fields given ");
  }
  if (inputs.size() < 2)
  {
    THROW_ALGORITHM_INPUT_ERROR(" At least 2 indicator functions stored as float values are needed to run cleaver! ");
  }

  std::ostringstream ostr0;
  ostr0 << "Be aware that inside and outside of materials (to be meshed) need to be defined as positive and negative (e.g. surface distance) values across all module inputs. The zero crossings represents material boundaries." << std::endl;
  remark(ostr0.str());

  detail::CleaverInputFieldList fields;

  VMesh::dimension_type dims; int x = 0, y = 0, z = 0;
  for (size_t p = 0; p < inputs.size(); p++)
  {
    auto input = inputs[p];
    auto imesh1 = input->vmesh();

    if (!imesh1->is_structuredmesh())
    {
      THROW_ALGORITHM_INPUT_ERROR("needs to be structured mesh!");
    }
    else
    {
      auto vfield1 = input->vfield();
      if (!vfield1->is_scalar())
      {
        THROW_ALGORITHM_INPUT_ERROR("values at the node needs to be scalar!");
      }

      imesh1->get_dimensions(dims);
      if (p == 0)
      {
        x = dims[0]; y = dims[1]; z = dims[2];
        if (x < 1 || y < 1 || z < 1)
        {
          THROW_ALGORITHM_INPUT_ERROR(" Size of input fields should be non-zero !");
        }
      }
      else
      {
        if (dims[0] != x || dims[1] != y || dims[2] != z)
        {
          THROW_ALGORITHM_INPUT_ERROR(" Size of input fields is inconsistent !");
        }
      }

      if (dims.size() != 3)
      {
        THROW_ALGORITHM_INPUT_ERROR("need a three dimensional indicator function");
      }

      //0 = constant, 1 = linear
      if (1 != vfield1->basis_order())
      {
        THROW_ALGORITHM_INPUT_ERROR("Input data need to be defined on input mesh nodes.");
      }

      if (vfield1->is_float())
      {
        auto ptr = static_cast<float*>(vfield1->fdata_pointer());
        if (ptr)
        {
          fields.push_back(detail::Cleaver2Impl::makeCleaver2FieldFromLatVol(input));
        }
        else
        {
          THROW_ALGORITHM_INPUT_ERROR("float field is NULL pointer");
        }
      }
      else
      {
        THROW_ALGORITHM_INPUT_ERROR("Input field needs to be a structured mesh (best would be a LatVol) with float values defined on mesh nodes. ");
      }

    }

  }

  detail::Cleaver2Parameters params
  {
    cleaver2::MeshType::Regular,
    detail::kDefaultAlphaLong,
    detail::kDefaultAlphaShort,
    get(Parameters::VolumeScaling).toDouble(),
    get(Parameters::Lipschitz).toDouble(),
    detail::kDefaultMultiplier,
    true
  };
  detail::Cleaver2Impl impl(this, params);
  return impl.doit(fields);
}

AlgorithmOutput InterfaceWithCleaver2Algorithm::run(const AlgorithmInput& input) const
{
  auto inputfields = input.getList<Field>(Variables::InputFields);

  auto output_fld = runImpl(inputfields);
  if (!output_fld)
    THROW_ALGORITHM_PROCESSING_ERROR("Null returned on legacy run call.");

  AlgorithmOutput output;
  output[Variables::OutputField] = output_fld;
  return output;
}
