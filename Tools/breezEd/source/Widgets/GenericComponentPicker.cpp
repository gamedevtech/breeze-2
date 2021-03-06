#include "stdafx.h"
#include "Widgets/GenericComponentPicker.h"

#include <QtWidgets/QFileDialog>
#include <QtCore/QDir>

#include <beCore/beComponentTypes.h>
#include <beCore/beReflectionTypes.h>

#include "Documents/SceneDocument.h"

#include "Editor.h"

#include "Widgets/ComponentPickerFactory.h"
#include "Plugins/FactoryManager.h"

#include "Utility/Strings.h"
#include "Utility/Checked.h"

namespace
{

/// Hides & removes the given widget from its layout.
void hideAndRemove(QWidget &widget)
{
	widget.hide();

	QWidget *pParent = widget.parentWidget();

	if (pParent)
	{
		QLayout *pLayout = pParent->layout();

		if (pLayout)
			pLayout->removeWidget(&widget);
	}
}

/// Recursively retrieves the last focus proxy in a chain.
QWidget* lastFocusProxy(QWidget *widget)
{
	QWidget *proxy = widget, *nextProxy = widget;

	while (nextProxy)
	{
		proxy = nextProxy;
		nextProxy = proxy->focusProxy();
	}

	return proxy;
}

/// Recursively checks if the given widget is a child of the given parent widget.
bool isChild(QWidget *widget, QWidget *parent)
{
	QWidget *intermediateParent = widget;

	while (intermediateParent)
	{
		if (intermediateParent == parent)
			return true;

		intermediateParent = intermediateParent->parentWidget();
	}

	return false;
}

/// Adapts the UI to match the given reflector.
void adaptUI(GenericComponentPicker &selector, Ui::GenericComponentPicker &ui, const beCore::ComponentReflector &reflector, bool bHasPrototype, Editor *pEditor)
{
	int optionCount = 0;

	uint4 componentFlags = reflector.GetComponentFlags();

	// Remove file option, if not available
	if (~componentFlags & bec::ComponentFlags::Filed)
	{
		hideAndRemove(*ui.fileButton);
		hideAndRemove(*ui.browseWidget);
	}
	else
	{
		ui.browseWidget->installFocusHandler(&selector);

		if (optionCount == 0)
		{
			ui.fileButton->setChecked(true);
			selector.setFocusProxy(ui.browseWidget);
		}
		++optionCount;
	}

	// Remove name option, if not available
	if (false)
	{
		hideAndRemove(*ui.nameButton);
		hideAndRemove(*ui.nameEdit);
	}
	else
	{
		ui.nameEdit->installEventFilter(&selector);

		if (optionCount == 0)
		{
			ui.nameButton->setChecked(true);
			selector.setFocusProxy(ui.nameEdit);
		}
		++optionCount;
	}

	// Recursively build parameter list
	if (componentFlags & bec::ComponentFlags::Creatable)
	{
		QWidget *firstParameterWidget = nullptr;

		beCore::ComponentParameters parameters = reflector.GetCreationParameters();

		for (beCore::ComponentParameters::iterator it = parameters.begin(); it != parameters.end(); ++it)
		{
			QGroupBox *parameterGroup = new QGroupBox( toQt(it->Name), &selector );
			QVBoxLayout *layout = new QVBoxLayout(parameterGroup);
			layout->setMargin(6);

			if (bHasPrototype && (it->Flags & bec::ComponentParameterFlags::Deducible) || it->Flags & bec::ComponentParameterFlags::Optional)
			{
				parameterGroup->setCheckable(true);
				parameterGroup->setChecked(true);
			}

			QWidget *parameterWidget;

			if (bec::GetReflectionType(it->Type) == bec::ReflectionType::String)
			{
				parameterWidget = new QLineEdit(parameterGroup);
				parameterWidget->installEventFilter(&selector);
			}
			else
			{
				const beCore::ComponentReflector *pReflector = beCore::GetComponentTypes().GetReflector(it->Type);

				if (pReflector)
				{
					const ComponentPickerFactory &componentPickerFactory = *LEAN_ASSERT_NOT_NULL(
							getComponentPickerFactories().getFactory( pReflector->GetType()->Name )
						);

					ComponentPicker *parameterPicker = componentPickerFactory.createComponentPicker(pReflector, nullptr, pEditor, parameterGroup);
					parameterPicker->installFocusHandler(&selector);
					parameterWidget = parameterPicker;
				}
				else
					parameterWidget = new QLabel(
							GenericComponentPicker::tr("Unknown type '%1'").arg( toQt(it->Type->Name) ),
							parameterGroup
						);
			}

			layout->addWidget(parameterWidget);

			if (!firstParameterWidget)
				firstParameterWidget = parameterWidget;

			parameterGroup->setLayout(layout);
			ui.newLayout->addWidget(parameterGroup);
		}

		// Reinsert clone check box at the end
		ui.newLayout->addWidget(ui.cloneCheckBox);

		ui.newWrapper->setFocusProxy(firstParameterWidget);

		if (optionCount == 0)
		{
			ui.newButton->setChecked(true);
			selector.setFocusProxy(ui.newWrapper);
		}
		++optionCount;
	}
	// Remove new option, if not available
	else
	{
		hideAndRemove(*ui.newButton);
		hideAndRemove(*ui.newWrapper);
	}

	// Don't show labels, if only one option
	if (optionCount == 1)
	{
		ui.fileButton->hide();
		ui.nameButton->hide();
		ui.newButton->hide();
	}

	selector.adjustSize();
}

/// Makes a component selector setting string.
QString makeSettingString(const beCore::ComponentReflector &reflector, const QString &setting)
{
	return QString("componentSelectorWidget/%1/%2").arg(reflector.GetType()->Name).arg(setting);
}

// Initialize the UI from current element or from the stored configuration.
void initUI(GenericComponentPicker &selector, Ui::GenericComponentPicker &ui, const beCore::ComponentReflector &reflector, const lean::any *pCurrent, Editor &editor)
{
	bool bFileInitialized = false;
	bool bNameInitialized = false;

	if (!pCurrent || ~reflector.GetComponentFlags() & bec::ComponentFlags::Cloneable)
		ui.cloneCheckBox->hide();

	if (pCurrent)
	{
		bec::ComponentInfo info = reflector.GetInfo(*pCurrent);

		if (!info.File.empty())
		{
			ui.browseWidget->setPath(toQt(info.File));
			bFileInitialized = true;

			ui.fileButton->setChecked(true);
			selector.setFocusProxy(ui.browseWidget);
		}
		else
		{
			ui.nameEdit->setText(toQt(info.Name));
			bNameInitialized = true;

			ui.nameButton->setChecked(true);
			selector.setFocusProxy(ui.nameEdit);
		}
	}

	if (!bFileInitialized)
		ui.browseWidget->setPath( editor.settings()->value(makeSettingString(reflector, "file"), QDir::currentPath()).toString() );
	if (!bNameInitialized)
		ui.nameEdit->setText( editor.settings()->value(makeSettingString(reflector, "name"), QString()).toString() );
}

} // namespace

// Constructor.
GenericComponentPicker::GenericComponentPicker(const beCore::ComponentReflector *pReflector, const lean::any *pCurrent, Editor *pEditor, QWidget *pParent, Qt::WindowFlags flags)
	: ComponentPicker(pParent, flags),
	ui( this ),
	m_pEditor( LEAN_ASSERT_NOT_NULL(pEditor) ),
	m_pReflector( LEAN_ASSERT_NOT_NULL(pReflector) ),
	m_pCurrent( pCurrent )
{
	// Remove irrelevant controls & recursively build widget
	adaptUI(*this, ui, *m_pReflector, m_pCurrent != nullptr, m_pEditor);

	// Initialize from current element or from stored configuration
	initUI(*this, ui, *m_pReflector, m_pCurrent, *m_pEditor);

	checkedConnect(ui.browseWidget, SIGNAL(browse()), this, SLOT(browse()));
}

// Destructor.
GenericComponentPicker::~GenericComponentPicker()
{
}

// Browser requested.
void GenericComponentPicker::browse()
{
	const ComponentPickerFactory &componentPickerFactory = *LEAN_ASSERT_NOT_NULL(
			getComponentPickerFactories().getFactory( m_pReflector->GetType()->Name )
		);

	// Browse for component using type-specific functionality
	QString file = componentPickerFactory.browseForComponent(*m_pReflector, ui.browseWidget->path(), *m_pEditor, this);

	if (!file.isEmpty())
	{
		ui.browseWidget->setPath(file);
		ui.fileButton->setChecked(true);
	}
}

// Gets the selected component.
lean::cloneable_obj<lean::any> GenericComponentPicker::acquireComponent(SceneDocument &document) const
{
	beCore::ParameterSet parameters = document.getSerializationParameters();

	if (ui.fileButton->isChecked())
	{
		QString path = ui.browseWidget->path();

		if (!path.isEmpty())
			m_pEditor->settings()->setValue(makeSettingString(*m_pReflector, "file"), path);

		return *m_pReflector->GetComponentByFile( toUtf8Range(path), beCore::Parameters(), parameters );
	}
	else if (ui.nameButton->isChecked())
	{
		QString name = ui.nameEdit->text();

		if (!name.isEmpty())
			m_pEditor->settings()->setValue(makeSettingString(*m_pReflector, "name"), name);

		return *m_pReflector->GetComponentByName( toUtf8Range(name), parameters );
	}
	else
	{
		beCore::Parameters creationParameters;

		int parameterCount = ui.newLayout->count();

		for (int idx = 0; idx < parameterCount; ++idx)
		{
			QGroupBox *pParameterGroup = qobject_cast<QGroupBox*>( ui.newLayout->itemAt(idx)->widget() );

			if (pParameterGroup && (!pParameterGroup->isCheckable() || pParameterGroup->isChecked()))
			{
				lean::utf8_string parameterName = toUtf8( pParameterGroup->title() );
				QWidget &parameterWidget = *LEAN_ASSERT_NOT_NULL( pParameterGroup->layout()->itemAt(0)->widget() );
				
				QLineEdit *pString = qobject_cast<QLineEdit*>(&parameterWidget);

				if (pString)
					creationParameters.SetValue<beCore::Exchange::utf8_string>(
							creationParameters.Add(parameterName),
							toUtf8Range(pString->text()).get().to<beCore::Exchange::utf8_string>()
						);
				else
				{
					ComponentPicker *pSubComponent = qobject_cast<ComponentPicker*>(&parameterWidget);

					if (pSubComponent)
						creationParameters.SetAnyValue( creationParameters.Add(parameterName), &*pSubComponent->acquireComponent(document) );
				}
			}
		}

		return *m_pReflector->CreateComponent( toUtf8Range(ui.newNameEdit->text()),
					creationParameters, parameters,
					ui.cloneCheckBox->isChecked() ? m_pCurrent : nullptr
				);
	}
}

// Filters focus events.
bool GenericComponentPicker::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::FocusIn)
	{
		QWidget *widget = static_cast<QWidget*>(obj);

		if (isChild(widget, ui.nameEdit))
			ui.nameButton->setChecked(true);
		else if (isChild(widget, ui.browseWidget))
			ui.fileButton->setChecked(true);
		else if (isChild(widget, ui.newWrapper))
			ui.newButton->setChecked(true);
	}

	return ComponentPicker::eventFilter(obj, event);
}

namespace
{

/// Plugin class.
struct GenericComponentPickerPlugin : public ComponentPickerFactory
{
	/// Constructor.
	GenericComponentPickerPlugin()
	{
		getComponentPickerFactories().setDefaultFactory(this);
	}

	/// Destructor.
	~GenericComponentPickerPlugin()
	{
		getComponentPickerFactories().unsetDefaultFactory(this);
	}

	/// Creates a component picker.
	ComponentPicker* createComponentPicker(const beCore::ComponentReflector *reflector, const lean::any *pCurrent, Editor *editor, QWidget *pParent) const
	{
		return new GenericComponentPicker(reflector, pCurrent, editor, pParent);
	}

	/// Browses for a component resource.
	virtual QString browseForComponent(const beCore::ComponentReflector &reflector, const QString &currentPath, Editor &editor, QWidget *pParent) const
	{
		QString location = (!currentPath.isEmpty()) ? currentPath : QDir::currentPath();

		// Open either breeze mesh or importable 3rd-party mesh format
		return QFileDialog::getOpenFileName( pParent,
			GenericComponentPicker::tr("Select a ''%1' resource").arg( reflector.GetType()->Name ),
				location,
				QString("%1 %2 (*.%3);;%4 (*.*)")
					.arg( reflector.GetType()->Name )
					.arg( GenericComponentPicker::tr("Files") )
					.arg( toQt(reflector.GetFileExtension()) )
					.arg( GenericComponentPicker::tr("All Files") )
			);
	}
};

const GenericComponentPickerPlugin GenericComponentPickerPlugin;

} // namespace
