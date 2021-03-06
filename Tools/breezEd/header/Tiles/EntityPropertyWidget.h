#ifndef ENTITYPROPERTYWIDGET_H
#define ENTITYPROPERTYWIDGET_H

#include "ui_EntityPropertyWidget.h"

#include <beEntitySystem/beEntities.h>

class QTimer;

class Editor;
class SceneDocument;
class AbstractDocument;

/// Entity builder tool.
class EntityPropertyWidget : public QWidget
{
	Q_OBJECT

public:
	/// Entity vector.
	typedef QVector<beEntitySystem::Entity*> entity_vector;

private:
	Ui::EntityPropertyWidget ui;

	Editor *m_pEditor;

	QTimer *m_pTimer;

	SceneDocument *m_pDocument;
	entity_vector m_selection;

public:
	/// Constructor.
	EntityPropertyWidget(Editor *pEditor, QWidget *pParent = nullptr , Qt::WindowFlags flags = 0);
	/// Destructor.
	~EntityPropertyWidget();

public Q_SLOTS:
	/// Sets the undo stack of the given document.
	void setDocument(AbstractDocument *pDocument);
	/// Sets the active selection of the given document.
	void setActiveSelection(SceneDocument *pDocument);

	/// Properties have changed.
	void propertiesChanged();
};

#endif
