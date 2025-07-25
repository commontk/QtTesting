// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqObjectNaming.h"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFocusFrame>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QScrollBar>
#include <QSet>
#include <QSignalMapper>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QtDebug>

namespace
{
QString ErrorMessage;
}

/** Returns the name of an object as if it was unnamed.*/
static const QString InternalGetNameAsUnnamed(QObject& Object)
{
  QString result;

  QObjectList siblings;
  if (Object.parent())
  {
    siblings = Object.parent()->children();
  }
  else
  {
    QWidgetList widgets = QApplication::topLevelWidgets();
    for (int i = 0; i != widgets.size(); ++i)
    {
      siblings.push_back(widgets[i]);
    }
  }

  const QString type = Object.metaObject()->className();

  // order of top level widgets is not guarenteed
  // we can someone counter that by checking visibility,
  // as we usually only test visible widgets, we would get the right one
  int invisible_index = 0;
  int visible_index = 0;
  for (int i = 0; i != siblings.size(); ++i)
  {
    QObject* test = siblings[i];
    if (test == &Object)
    {
      break;
    }
    else if (type == test->metaObject()->className() && test->objectName().isEmpty())
    {
      QWidget* widget = qobject_cast<QWidget*>(test);
      if (widget && widget->isVisible())
      {
        ++visible_index;
      }
      else
      {
        ++invisible_index;
      }
    }
  }

  int index = invisible_index;
  if (QWidget* const widget = qobject_cast<QWidget*>(&Object))
  {
    if (widget->isVisible())
    {
      result += QString::number(1);
      index = visible_index;
    }
    else
    {
      result += QString::number(0);
    }
  }

  result += type + QString::number(index);

  result.replace("/", "|");
  return result;
}

/** Returns the name of an object.  If the object doesn't have an explicit name,
assigns a name as a convenience.  Also replaces problematic characters such as '/'.
*/
static const QString InternalGetName(QObject& Object)
{
  QString result = Object.objectName();

  if (result.isEmpty())
  {
    result = InternalGetNameAsUnnamed(Object);
  }

  if (qobject_cast<QApplication*>(&Object))
  {
    result.append("-app");
  }

  result.replace("/", "|");
  return result;
}

const QString pqObjectNaming::GetName(QObject& Object)
{
  QString name = InternalGetName(Object);
  if (name.isEmpty())
  {
    qCritical() << "Cannot record event for unnamed object " << &Object;
    return QString();
  }

  for (QObject* p = Object.parent(); p; p = p->parent())
  {
    const QString parent_name = InternalGetName(*p);

    if (parent_name.isEmpty())
    {
      qCritical() << "Cannot record event for incompletely-named object " << name << " " << &Object
                  << " with parent " << p;
      return QString();
    }

    name = parent_name + "/" + name;

    if (!p->parent() && !QApplication::topLevelWidgets().contains(qobject_cast<QWidget*>(p)))
    {
      qCritical() << "Unable to to determine name for Object " << &Object << " because a parent "
                  << p << " is not a top-level widget. Name so far = " << name;
      return QString();
    }
  }

  return name;
}

QObject* pqObjectNaming::GetObject(const QString& Name)
{
  QObject* result = 0;
  QObject* lastObject = 0;
  if (Name.isEmpty())
  {
    return 0;
  }

  const QStringList names = Name.split("/");

  // see if QApplication is the requested object
  QString app_name = InternalGetName(*QApplication::instance());
  if (app_name == Name)
  {
    return QApplication::instance();
  }

  const QWidgetList top_level_widgets = QApplication::topLevelWidgets();
  for (int i = 0; i != top_level_widgets.size(); ++i)
  {
    QObject* object = top_level_widgets[i];
    const QString name = InternalGetName(*object);
    const QString alt_name = InternalGetNameAsUnnamed(*object);

    if (name == names[0] || alt_name == names[0])
    {
      result = object;
      lastObject = object;
      break;
    }
  }

  for (int j = 1; j < names.size(); ++j)
  {
    const QObjectList& children = result ? result->children() : QObjectList();

    result = 0;
    QString objectName = names[j];
    for (int k = 0; k != children.size(); ++k)
    {
      QObject* child = children[k];
      const QString name = InternalGetName(*child);
      const QString alt_name = InternalGetNameAsUnnamed(*child);

      if (name == objectName || alt_name == objectName)
      {
        result = child;
        lastObject = child;
        break;
      }

      // Sometimes, when playing, widget are visible when they were not during recording,
      // try again with visibility at 1.
      if (k == children.size() - 1 && result == 0 && !objectName.isEmpty() && objectName[0] == '0')
      {
        objectName[0] = '1';
        k = 0;
      }
    }
  }

  if (result)
    return result;

  ErrorMessage.clear();
  QTextStream stream(&ErrorMessage);
  stream << "\n"; // a newline to keep horizontal alignment
  stream << "Couldn't find object  `" << Name << "`\n";
  if (lastObject)
  {
    stream << "Found up to           `" << pqObjectNaming::GetName(*lastObject) << "`\n";
  }

  // controls how many matches to dump in error message.
  QString matchLimitEnv = QString::fromUtf8(qgetenv("PQOBJECTNAMING_MATCH_LIMIT"));
  const int matchLimit = matchLimitEnv.isEmpty() ? 20 : matchLimitEnv.toInt();

  bool foundMatch = false;
  if (lastObject)
  {
    QObjectList matches = lastObject->findChildren<QObject*>(names[names.size() - 1]);
    for (int cc = 0; (matchLimit <= 0 || cc < matchLimit) && cc < matches.size(); ++cc)
    {
      stream << "    Possible match:   `" << pqObjectNaming::GetName(*matches[cc]) << "`\n";
      foundMatch = true;
    }
    if (matchLimit > 0 && matches.size() > matchLimit)
    {
      stream << "    Possible match: .... (and " << (matches.size() - matchLimit) << " more!)\n"
             << "    Set PQOBJECTNAMING_MATCH_LIMIT environment var to a +'ve number to limit "
                "entries (or 0 for unlimited).\n";
    }
    if (!foundMatch)
    {
      matches = lastObject->findChildren<QObject*>();
      for (int cc = 0; (matchLimit <= 0 || cc < matchLimit) && cc < matches.size(); ++cc)
      {
        stream << "    Available widget: `" << pqObjectNaming::GetName(*matches[cc]) << "`\n";
      }
      if (matchLimit > 0 && matches.size() > matchLimit)
      {
        stream << "    Available widget: .... (and " << (matches.size() - matchLimit) << " more!)\n"
               << "    Set PQOBJECTNAMING_MATCH_LIMIT environment var to a +'ve number to limit "
                  "entries (or 0 for unlimited).\n";
      }
    }
  }
  return 0;
}

void pqObjectNaming::DumpHierarchy(QStringList& results)
{
  const QWidgetList widgets = QApplication::topLevelWidgets();
  for (int i = 0; i != widgets.size(); ++i)
  {
    DumpHierarchy(*widgets[i], results);
  }
}

void pqObjectNaming::DumpHierarchy(QObject& object, QStringList& results)
{
  results << GetName(object);

  const QObjectList children = object.children();
  for (int i = 0; i != children.size(); ++i)
  {
    DumpHierarchy(*children[i], results);
  }
}

QString pqObjectNaming::lastErrorMessage()
{
  return ErrorMessage;
}
