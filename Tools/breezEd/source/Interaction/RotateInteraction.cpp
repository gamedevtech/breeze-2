#include "stdafx.h"
#include "Interaction/RotateInteraction.h"

#include "Commands/RotateEntity.h"

#include "Utility/InputProvider.h"

#include "Interaction/Queries.h"
#include "Documents/SceneDocument.h"

#include "DeviceManager.h"

#include <beEntitySystem/beEntity.h>
#include "Interaction/Widgets.h"
#include "Interaction/Math.h"

#include <beMath/beVector.h>
#include <beMath/beMatrix.h>
#include <beMath/bePlane.h>

#include "Utility/Checked.h"

#include <lean/logging/errors.h>

static const uint4 RotateAxisWidgetIDs[3] = { reserveWidgetID(), reserveWidgetID(), reserveWidgetID() };

// Constructor.
RotateInteraction::RotateInteraction(SceneDocument *pDocument, QObject *pParent)
	: QObject( pParent ),
	m_pDocument( LEAN_ASSERT_NOT_NULL(pDocument) ),
	m_pCommand(),
	m_axisID( InvalidAxisID )
{
	// TODO: NO HARD-CODED PATHS
	m_axes[0] = createWidgetMesh(RotateAxisWidgetIDs[0], "Static/UI/RotateWidget.mesh", beMath::vec(1.0f, 0.0f, 0.1f, 0.7f),
		m_pDocument->scene(), *m_pDocument->deviceManager()->graphicsResources(), *m_pDocument->renderer());
	m_axes[1] = createWidgetMesh(RotateAxisWidgetIDs[1], "Static/UI/RotateWidget.mesh", beMath::vec(0.1f, 1.0f, 0.0f, 0.7f),
		m_pDocument->scene(), *m_pDocument->deviceManager()->graphicsResources(), *m_pDocument->renderer());
	m_axes[2] = createWidgetMesh(RotateAxisWidgetIDs[2], "Static/UI/RotateWidget.mesh", beMath::vec(0.0f, 0.1f, 1.0f, 0.7f),
		m_pDocument->scene(), *m_pDocument->deviceManager()->graphicsResources(), *m_pDocument->renderer());
}

// Destructor.
RotateInteraction::~RotateInteraction()
{
}

// Steps the interaction.
void RotateInteraction::step(float timeStep, InputProvider &input, const beScene::PerspectiveDesc &perspective)
{
	using namespace beMath;

	uint4 axisID = InvalidAxisID;

	// Keep / stop moving
	if (input.buttonPressed(Qt::LeftButton))
	{
		axisID = m_axisID;

		// Check if user picked new axis
		if (axisID == InvalidAxisID && input.buttonPressed(Qt::LeftButton, true))
		{
			uint4 objectID = objectIDUnderCursor(*m_pDocument->renderer(), *m_pDocument->scene(), input.relativePosition(), perspective);

			for (int i = 0; i < 3; ++i)
				if (objectID == RotateAxisWidgetIDs[i])
					axisID = i;
		}

		// Moving
		if (axisID != InvalidAxisID)
			input.setButtonHandled(Qt::LeftButton);
	}
	else
	{
		m_pCommand = nullptr;
		m_axisID = InvalidAxisID;
	}

	if (axisID < 3)
	{
		fvec3 axisOrig = m_axes[axisID]->GetPosition();
		fvec3 axisDir = m_axes[axisID]->GetOrientation()[2];

		fvec3 rayOrig, rayDir;
		camRayDirUnderCursor(rayOrig, rayDir, toQt(input.relativePosition()), perspective.ViewProjMat);

		// Compute point of intersection
		fplane3 rotatePlane = mkplane(axisDir, axisOrig);
		fvec3 nextAxisStop = rayOrig + rayDir * intersect(rotatePlane, rayOrig, rayDir);

		// Perform rotate operation
		if (m_axisID != InvalidAxisID)
		{
			fvec3 rotateVec = normalize(m_axisStop - axisOrig);
			fvec3 nextRotateVec = normalize(nextAxisStop - axisOrig);

			float rotationAngle = acos( min( max( dot(rotateVec, nextRotateVec), -1.0f), 1.0f ) );

			if ( dot( cross(rotateVec, nextRotateVec), axisDir ) < 0.0f )
				rotationAngle = -rotationAngle;

			fmat3 rotationDelta = mat_rot<3>(axisDir, rotationAngle);

			for (entity_vector::const_iterator itEntity = m_selection.begin(); itEntity != m_selection.end(); ++itEntity)
			{
				fvec3 position = (*itEntity)->GetPosition();
				fmat3 orientation = (*itEntity)->GetOrientation();
				
				position += m_centroid + mul(position - m_centroid, rotationDelta) - position;
				orientation = mul(orientation, rotationDelta);
				
				(*itEntity)->SetPosition(position);
				(*itEntity)->SetOrientation(orientation);
			}
			
			m_axisStop = nextAxisStop;
			m_pCommand->capture();
			updateWidgets();
		}
		// Initialize rotate operation
		else
		{
			m_pCommand = new RotateEntityCommand(m_selection);
			m_pDocument->undoStack()->push(m_pCommand);

			m_axisStop = nextAxisStop;
			m_axisID = axisID;
		}
	}
}

// Attaches this interaction.
void RotateInteraction::attach()
{
	for (int i = 0; i < 3; ++i)
		m_axes[i]->Attach();

	checkedConnect(m_pDocument, SIGNAL(selectionChanged(SceneDocument*)), this, SLOT(updateWidgets()));
}

// Detaches this interaction.
void RotateInteraction::detach()
{
	for (int i = 0; i < 3; ++i)
		m_axes[i]->Detach();

	disconnect(m_pDocument, SIGNAL(selectionChanged(SceneDocument*)), this, SLOT(updateWidgets()));
}

// Update widgets.
void RotateInteraction::updateWidgets()
{
	m_selection = m_pDocument->selection();

	beMath::fmat3 orientation = (m_selection.size() == 1)
		? m_selection.front()->GetOrientation()
		: beMath::mat_diag<3, 3>(1.0f);
	
	m_centroid = 0.0f;

	for (entity_vector::const_iterator itEntity = m_selection.begin(); itEntity != m_selection.end(); ++itEntity)
		m_centroid += (*itEntity)->GetPosition();

	m_centroid /= m_selection.size();

	for (int i = 0; i < 3; ++i)
	{
		m_axes[i]->SetPosition(m_centroid);
		m_axes[i]->SetOrientation(
				beMath::mat_row(
					orientation[ (i + 1) % 3 ],
					orientation[ (i + 2) % 3 ],
					orientation[ (i + 0) % 3 ]
				)
			);
	}

	// TODO: show / hide?
}