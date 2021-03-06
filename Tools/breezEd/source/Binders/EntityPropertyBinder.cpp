#include "stdafx.h"
#include "Binders/EntityPropertyBinder.h"
#include "Binders/GenericPropertyBinder.h"

#include <QtWidgets/QTreeView>

#include <beEntitySystem/beEntityController.h>

#include "Utility/Strings.h"
#include "Utility/Checked.h"

// Constructor.
EntityPropertyBinder::EntityPropertyBinder(beEntitySystem::Entity *pEntity,
										   SceneDocument *pDocument,
										   QTreeView *pTree, QStandardItem *pParentItem,
										   QObject *pParent)
	: QObject(pParent),
	m_pEntity( LEAN_ASSERT_NOT_NULL(pEntity) )
{
	// Create new empty model, if none to be extended
	if (!pParentItem)
	{
		GenericPropertyBinder::setupTree(*pTree);

		// WARNING: Don't attach to binder, binder might be attached to model
		QStandardItemModel *pModel = new QStandardItemModel(pTree);
		GenericPropertyBinder::setupModel(*pModel);
		pTree->setModel(pModel);

		pParentItem = pModel->invisibleRootItem();
	}

	QStandardItem *pEntityItem = new QStandardItem( pEntity->GetType()->Name );
	pEntityItem->setFlags(Qt::ItemIsEnabled);
	pParentItem->appendRow(pEntityItem);
	GenericPropertyBinder::fillRow(*pEntityItem);
	
	GenericPropertyBinder *pEntityBinder = new GenericPropertyBinder(m_pEntity, m_pEntity, pDocument, pTree, pEntityItem, this);
	checkedConnect(this, SIGNAL(propagateUpdateProperties()), pEntityBinder, SLOT(updateProperties()));
	checkedConnect(pEntityBinder, SIGNAL(propertiesChanged()), this, SIGNAL(propertiesChanged()));
	pTree->expand( pEntityItem->index() );

	beEntitySystem::Entity::Controllers controllers = m_pEntity->GetControllers();

	for (beEntitySystem::Entity::Controllers controllers = m_pEntity->GetControllers();
		controllers.Begin < controllers.End; ++controllers.Begin)
	{
		beEntitySystem::EntityController *pController = *controllers.Begin;

		QStandardItem *pControllerItem = new QStandardItem( pController->GetType()->Name );
		pControllerItem->setFlags(Qt::ItemIsEnabled);
		pParentItem->appendRow( pControllerItem );
		GenericPropertyBinder::fillRow(*pControllerItem);

		GenericPropertyBinder *pBinder = new GenericPropertyBinder(pController, pController, pDocument, pTree, pControllerItem, this);
		checkedConnect(this, SIGNAL(propagateUpdateProperties()), pBinder, SLOT(updateProperties()));
		checkedConnect(pBinder, SIGNAL(propertiesChanged()), this, SIGNAL(propertiesChanged()));
		pTree->expand( pControllerItem->index() );
	}
}

// Destructor.
EntityPropertyBinder::~EntityPropertyBinder()
{
}

// Check for property changes.
void EntityPropertyBinder::updateProperties()
{
	Q_EMIT propagateUpdateProperties();
}
