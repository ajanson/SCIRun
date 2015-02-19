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

#ifndef INTERFACE_APPLICATION_PORTWIDGET_H
#define INTERFACE_APPLICATION_PORTWIDGET_H

#include <boost/shared_ptr.hpp>
#include <QGraphicsWidget>
#include <QPushButton>
#include <QColor>
#include <set>
#include <Interface/Application/PositionProvider.h>
#include <Dataflow/Network/PortInterface.h>
#include <Dataflow/Network/ConnectionId.h>

class QGraphicsScene;

namespace SCIRun {
namespace Gui {

class ConnectionLine;
class PositionProvider;
class ConnectionInProgress;
class ConnectionFactory;
class ClosestPortFinder;
class PortActionsMenu;

class PortWidgetBase : public QPushButton, public SCIRun::Dataflow::Networks::PortDescriptionInterface
{
public:
  virtual SCIRun::Dataflow::Networks::PortId id() const = 0;
  virtual size_t nconnections() const = 0;
  virtual std::string get_typename() const = 0;
  virtual std::string get_portname() const = 0;
  virtual bool isInput() const = 0;
  virtual bool isDynamic() const = 0;
  virtual SCIRun::Dataflow::Networks::ModuleId getUnderlyingModuleId() const = 0;
  virtual size_t getIndex() const = 0;

  virtual QColor color() const = 0;
  virtual bool isLightOn() const = 0;

  static const int WIDTH = 11;

protected:
  explicit PortWidgetBase(QWidget* parent);
  virtual QSize sizeHint() const override;
  virtual void paintEvent(QPaintEvent* event) override;
};

class PortWidget : public PortWidgetBase, public NeedsScenePositionProvider
{
  Q_OBJECT
public:
  PortWidget(const QString& name, const QColor& color, const std::string& datatype, const SCIRun::Dataflow::Networks::ModuleId& moduleId,
    const SCIRun::Dataflow::Networks::PortId& portId, size_t index, bool isInput, bool isDynamic,
    boost::shared_ptr<ConnectionFactory> connectionFactory,
    boost::shared_ptr<ClosestPortFinder> closestPortFinder,
    SCIRun::Dataflow::Networks::PortDataDescriber portDataDescriber,
    QWidget* parent = 0);
  virtual ~PortWidget();

  QString name() const { return name_; }
  virtual QColor color() const override { return color_; }
  virtual bool isInput() const { return isInput_; }
  virtual bool isDynamic() const { return isDynamic_; }
  bool isConnected() const { return isConnected_; }
  void setConnected(bool connected) { isConnected_ = connected; }

  virtual size_t nconnections() const;
  virtual std::string get_typename() const;
  virtual std::string get_portname() const;
  virtual Dataflow::Networks::ModuleId getUnderlyingModuleId() const;
  virtual size_t getIndex() const;
  void setIndex(size_t i);

  virtual SCIRun::Dataflow::Networks::PortId id() const;

  void toggleLight();
  void turn_on_light();
  void turn_off_light();
  virtual bool isLightOn() const override { return lightOn_; }

  void setHighlight(bool on);

  void addConnection(ConnectionLine* c);
  void removeConnection(ConnectionLine* c);

  void trackConnections();
  void deleteConnections();

  QPointF position() const;

  bool sharesParentModule(const PortWidget& other) const;
  bool isFullInputPort() const;

  void doMousePress(Qt::MouseButton button, const QPointF& pos);
  void doMouseMove(Qt::MouseButtons buttons, const QPointF& pos);
  void doMouseRelease(Qt::MouseButton button, const QPointF& pos, Qt::KeyboardModifiers modifiers);

  SCIRun::Dataflow::Networks::PortDataDescriber getPortDataDescriber() const { return portDataDescriber_; }

protected:
  virtual void moveEvent(QMoveEvent * event);

public Q_SLOTS:
  void MakeTheConnection(const SCIRun::Dataflow::Networks::ConnectionDescription& cd);
  void cancelConnectionsInProgress();
  void portCachingChanged(bool checked);
  void connectNewModule();
Q_SIGNALS:
  void requestConnection(const SCIRun::Dataflow::Networks::PortDescriptionInterface* from, const SCIRun::Dataflow::Networks::PortDescriptionInterface* to);
  void connectionDeleted(const SCIRun::Dataflow::Networks::ConnectionId& id);
  void connectNewModule(const SCIRun::Dataflow::Networks::PortDescriptionInterface* portToConnect, const std::string& newModuleName);
  void portMoved();
  void connectionNoteChanged();
protected:
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
private:
  void performDrag(const QPointF& endPos);
  void makeConnection(const QPointF& pos);
  void tryConnectPort(const QPointF& pos, PortWidget* port);
  bool matches(const SCIRun::Dataflow::Networks::ConnectionDescription& cd) const;

  const QString name_;
  const SCIRun::Dataflow::Networks::ModuleId moduleId_;
  const SCIRun::Dataflow::Networks::PortId portId_;
  size_t index_;
  const QColor color_;
  const std::string typename_;
  const bool isInput_;
  const bool isDynamic_;
  bool isConnected_;
  bool lightOn_;
  QPointF startPos_;
  ConnectionInProgress* currentConnection_;
  friend struct DeleteCurrentConnectionAtEndOfBlock;
  std::set<ConnectionLine*> connections_;
  boost::shared_ptr<ConnectionFactory> connectionFactory_;
  boost::shared_ptr<ClosestPortFinder> closestPortFinder_;
  PortActionsMenu* menu_;
  SCIRun::Dataflow::Networks::PortDataDescriber portDataDescriber_;
  //TODO
  typedef std::map<std::string, std::map<bool, std::map<SCIRun::Dataflow::Networks::PortId, PortWidget*>>> PortWidgetMap;
  static PortWidgetMap portWidgetMap_;
};

// To fill the layout
class BlankPort : public PortWidgetBase
{
public:
  explicit BlankPort(QWidget* parent);
  virtual SCIRun::Dataflow::Networks::PortId id() const;
  virtual size_t nconnections() const { return 0; }
  virtual std::string get_typename() const { return ""; }
  virtual std::string get_portname() const { return "<Blank>"; }
  virtual bool isInput() const { return false; }
  virtual bool isDynamic() const { return false; }
  virtual SCIRun::Dataflow::Networks::ModuleId getUnderlyingModuleId() const;// { return "<Blank>"; }
  virtual size_t getIndex() const { return 0; }

  virtual QColor color() const;
  virtual bool isLightOn() const { return false; }
};

class InputPortWidget : public PortWidget
{
public:
  InputPortWidget(const QString& name, const QColor& color, const std::string& datatype, const SCIRun::Dataflow::Networks::ModuleId& moduleId,
    const SCIRun::Dataflow::Networks::PortId& portId, size_t index, bool isDynamic,
    boost::shared_ptr<ConnectionFactory> connectionFactory,
    boost::shared_ptr<ClosestPortFinder> closestPortFinder,
    SCIRun::Dataflow::Networks::PortDataDescriber portDataDescriber,
    QWidget* parent = 0);
};

class OutputPortWidget : public PortWidget
{
public:
  OutputPortWidget(const QString& name, const QColor& color, const std::string& datatype, const SCIRun::Dataflow::Networks::ModuleId& moduleId,
    const SCIRun::Dataflow::Networks::PortId& portId, size_t index, bool isDynamic,
    boost::shared_ptr<ConnectionFactory> connectionFactory,
    boost::shared_ptr<ClosestPortFinder> closestPortFinder,
    SCIRun::Dataflow::Networks::PortDataDescriber portDataDescriber,
    QWidget* parent = 0);
};

class DataInfoDialog
{
public:
  static void show(SCIRun::Dataflow::Networks::PortDataDescriber portDataDescriber, const QString& label, const std::string& id);
};

}
}

#endif
