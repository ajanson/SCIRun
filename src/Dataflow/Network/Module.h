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


#ifndef DATAFLOW_NETWORK_MODULE_H
#define DATAFLOW_NETWORK_MODULE_H 

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <Dataflow/Network/NetworkFwd.h>
#include <Dataflow/Network/ModuleInterface.h>
#include <Dataflow/Network/ModuleStateInterface.h>
#include <Dataflow/Network/ModuleDescription.h>
#include <Dataflow/Network/PortManager.h>
#include <Dataflow/Network/Share.h>

namespace SCIRun {
namespace Dataflow {
namespace Networks {
  
  class SCISHARE Module : public ModuleInterface, boost::noncopyable
  {
  public:
    Module(const ModuleLookupInfo& info, 
      ModuleStateFactoryHandle stateFactory = defaultStateFactory_,
      bool hasUi = true, 
      const std::string& version = "1.0");
    virtual ~Module();

    std::string get_module_name() const { return info_.module_name_; }
    std::string get_categoryname() const { return info_.category_name_; }
    std::string get_packagename() const { return info_.package_name_; }
    std::string get_id() const { return id_; }

    //for serialization
    virtual const ModuleLookupInfo& get_info() const { return info_; }
    virtual void set_id(const std::string& id) { id_ = id; }

    bool has_ui() const { return has_ui_; }
    size_t num_input_ports() const;
    size_t num_output_ports() const;

    InputPortHandle get_input_port(const std::string &name) const;
    OutputPortHandle get_output_port(const std::string &name) const;
    OutputPortHandle get_output_port(size_t idx) const;
    InputPortHandle get_input_port(size_t idx) const;

    //TODO: execute signal here.
    virtual void do_execute();
    virtual ModuleStateHandle get_state();
    virtual void set_state(ModuleStateHandle state);

    virtual SCIRun::Core::Datatypes::DatatypeHandleOption get_input_handle(size_t idx);
    virtual void send_output_handle(size_t idx, SCIRun::Core::Datatypes::DatatypeHandle data);

    virtual void setLogger(SCIRun::Core::Logging::LoggerHandle log) { log_ = log; }
    virtual SCIRun::Core::Logging::LoggerHandle getLogger() const { return log_; }

    // Throws if input is not present or null.
    template <class T>
    boost::shared_ptr<const T> getRequiredInput(size_t idx);

    template <class T>
    boost::shared_ptr<const T> getRequiredInput(const std::string& name);

    class SCISHARE Builder : boost::noncopyable
    {
    public:
      Builder();
      Builder& with_name(const std::string& name);
      Builder& using_func(ModuleMaker create);
      Builder& add_input_port(const Port::ConstructionParams& params);
      Builder& add_output_port(const Port::ConstructionParams& params);
      Builder& disable_ui();
      ModuleHandle build();

      typedef boost::function<SCIRun::Dataflow::Networks::DatatypeSinkInterface*()> SinkMaker;
      typedef boost::function<SCIRun::Dataflow::Networks::DatatypeSourceInterface*()> SourceMaker;
      static void use_sink_type(SinkMaker func);
      static void use_source_type(SourceMaker func);
    private:
      boost::shared_ptr<Module> module_;
      static SinkMaker sink_maker_;
      static SourceMaker source_maker_;
    };

    //TODO: yuck
    static ModuleStateFactoryHandle defaultStateFactory_;

  protected:
    ModuleLookupInfo info_;

    double executionTime_;
    std::string id_;

  private:
    friend class Builder;
    void add_input_port(InputPortHandle);
    void add_output_port(OutputPortHandle);
    bool has_ui_;
   
    ModuleStateHandle state_;
    PortManager<OutputPortHandle> oports_;
    PortManager<InputPortHandle> iports_;

    SCIRun::Core::Logging::LoggerHandle log_;
    static int instanceCount_;
    static SCIRun::Core::Logging::LoggerHandle defaultLogger_;
  };

  struct SCISHARE DataPortException : virtual Core::ExceptionBase {};
  struct SCISHARE NoHandleOnPortException : virtual DataPortException {};
  struct SCISHARE NullHandleOnPortException : virtual DataPortException {};
  struct SCISHARE WrongDatatypeOnPortException : virtual DataPortException {};
  struct SCISHARE PortNotFoundException : virtual DataPortException {};

  template <class T>
  boost::shared_ptr<const T> Module::getRequiredInput(size_t idx)
  {
    auto inputOpt = get_input_handle(idx);
    if (!inputOpt)
      BOOST_THROW_EXCEPTION(NoHandleOnPortException() << Core::ErrorMessage("Input data required on port # " + boost::lexical_cast<std::string>(idx)));

    if (!*inputOpt)
      BOOST_THROW_EXCEPTION(NullHandleOnPortException());

    boost::shared_ptr<const T> data = boost::dynamic_pointer_cast<const T>(*inputOpt);
    if (!data)
    {
      BOOST_THROW_EXCEPTION(WrongDatatypeOnPortException());
    }
    return data;
  }
  
  
}}


namespace Modules
{
  template <class PortTypeTag>
  class Has1InputPort
  {
  public:
    static SCIRun::Dataflow::Networks::InputPortDescription inputPortDescription(const std::string& port0Name);
  };

  template <class PortTypeTag0, class PortTypeTag1>
  class Has2InputPorts
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::InputPortDescription> inputPortDescription(const std::string& port0Name, const std::string& port1Name)
    {
      std::vector<SCIRun::Dataflow::Networks::InputPortDescription> ports;
      ports.push_back(Has1InputPort<PortTypeTag0>::inputPortDescription(port0Name));
      ports.push_back(Has1InputPort<PortTypeTag1>::inputPortDescription(port1Name));
      return ports;
    }
  };

  struct SCISHARE MatrixPortTag {};
  struct SCISHARE ScalarPortTag {};
  struct SCISHARE StringPortTag {};
  struct SCISHARE FieldPortTag {};
  struct SCISHARE GeometryPortTag {};

  inline SCIRun::Dataflow::Networks::PortDescription MakeMatrixPort(const std::string& name)
  {
    return SCIRun::Dataflow::Networks::PortDescription(name, "Matrix", "blue"); 
  }

  inline SCIRun::Dataflow::Networks::PortDescription MakeScalarPort(const std::string& name)
  {
    return SCIRun::Dataflow::Networks::PortDescription(name, "Scalar", "white"); 
  }

  inline SCIRun::Dataflow::Networks::PortDescription MakeStringPort(const std::string& name)
  {
    return SCIRun::Dataflow::Networks::PortDescription(name, "String", "darkGreen"); 
  }

  inline SCIRun::Dataflow::Networks::PortDescription MakeFieldPort(const std::string& name)
  {
    return SCIRun::Dataflow::Networks::PortDescription(name, "Field", "yellow"); 
  }

  inline SCIRun::Dataflow::Networks::PortDescription MakeGeometryPort(const std::string& name)
  {
    return SCIRun::Dataflow::Networks::PortDescription(name, "Geometry", "magenta"); 
  }

#define INPUT_PORT_SPEC(name)   template <>\
  class Has1InputPort<name ##PortTag>\
  {\
  public:\
    static SCIRun::Dataflow::Networks::InputPortDescription inputPortDescription(const std::string& port0Name)\
    {\
      return Make ## name ## Port(port0Name); \
    }\
  }\

  INPUT_PORT_SPEC(Matrix);
  INPUT_PORT_SPEC(Scalar);
  INPUT_PORT_SPEC(String);
  INPUT_PORT_SPEC(Field);
  INPUT_PORT_SPEC(Geometry);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////

  template <class PortTypeTag>
  class Has1OutputPort
  {
  public:
    static SCIRun::Dataflow::Networks::OutputPortDescription outputPortDescription(const std::string& port0Name);
  };

  template <class PortTypeTag0, class PortTypeTag1>
  class Has2OutputPorts
  {
  public:
    static std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> outputPortDescription(const std::string& port0Name, const std::string& port1Name)
    {
      std::vector<SCIRun::Dataflow::Networks::OutputPortDescription> ports;
      ports.push_back(Has1OutputPort<PortTypeTag0>::outputPortDescription(port0Name));
      ports.push_back(Has1OutputPort<PortTypeTag1>::outputPortDescription(port1Name));
      return ports;
    }
  };

  template <>
  class Has1OutputPort<MatrixPortTag>
  {
  public:
    static SCIRun::Dataflow::Networks::OutputPortDescription outputPortDescription(const std::string& port0Name)
    {
      return MakeMatrixPort(port0Name); 
    }
  };

  template <>
  class Has1OutputPort<ScalarPortTag>
  {
  public:
    static SCIRun::Dataflow::Networks::OutputPortDescription outputPortDescription(const std::string& port0Name)
    {
      return MakeScalarPort(port0Name); 
    }
  };
  
  template <>
  class Has1OutputPort<StringPortTag>
  {
  public:
    static SCIRun::Dataflow::Networks::OutputPortDescription outputPortDescription(const std::string& port0Name)
    {
      return MakeStringPort(port0Name); 
    }
  };

  template <>
  class Has1OutputPort<FieldPortTag>
  {
  public:
    static SCIRun::Dataflow::Networks::OutputPortDescription outputPortDescription(const std::string& port0Name)
    {
      return MakeFieldPort(port0Name); 
    }
  };

  template <>
  class Has1OutputPort<GeometryPortTag>
  {
  public:
    static SCIRun::Dataflow::Networks::OutputPortDescription outputPortDescription(const std::string& port0Name)
    {
      return MakeGeometryPort(port0Name); 
    }
  };
}
}

#endif
