#include "stdafx.h"
#include "Windows/MainWindow.h"

#include "Editor.h"
#include "Plugins/PluginManager.h"

#include "Windows/NewDocumentDialog.h"
#include "Documents/DocumentManager.h"
#include "Documents/AbstractDocument.h"
#include "Documents/AbstractDocumentFactory.h"

#include "Modes/Mode.h"
#include "Windows/MdiDocumentWindow.h"

#include "Views/AbstractView.h"

#include <QtWidgets/QFileDialog>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QSettings>

#include "Windows/WindowsDialog.h"
#include "Tiles/ConsoleWidget.h"

#include "Docking/DockContainer.h"

#include "Utility/UI.h"
#include "Utility/Checked.h"

#include <lean/logging/errors.h>

namespace
{

/// Restores the docking layout.
void restoreLayout(QMainWindow &window, DockContainer &dock, const Editor &editor)
{
	window.restoreState(editor.settings()->value("mainWindow/windowState").toByteArray());
	dock.restoreState(editor.settings()->value("mainWindow/dockState").toByteArray());
}

/// Initializes window state and docking layout.
void restoreWindow(QMainWindow &window, DockContainer &dock, const Editor &editor)
{
	window.restoreGeometry(editor.settings()->value("mainWindow/geometry").toByteArray());
	restoreLayout(window, dock, editor);
}

/// Saves docking layout.
void saveLayout(const QMainWindow &window, const DockContainer &dock, Editor &editor)
{
	editor.settings()->setValue("mainWindow/windowState", window.saveState());
	editor.settings()->setValue("mainWindow/dockState", dock.saveState());
}

/// Saves window state and docking layout.
void saveWindow(const QMainWindow &window, const DockContainer &dock, Editor &editor)
{
	saveLayout(window, dock, editor);
	editor.settings()->setValue("mainWindow/geometry", window.saveGeometry());
}

/// Builds the open as menu.
void addDocumentTypes(MainWindow &mainWindow, Editor &editor)
{
	QList<DocumentType> documentTypes = editor.documentManager()->documentTypes();

	// Create 'Open As'-menu
	QMenu *pOpenMenu = new QMenu(&mainWindow);
	mainWindow.widgets().actionOpen_As->setMenu(pOpenMenu);

	QSignalMapper *pSignalMapper = new QSignalMapper(pOpenMenu);

	Q_FOREACH (const DocumentType &documentType, documentTypes)
	{
		// Add one open action for each document type
		QAction *pAction = pOpenMenu->addAction(documentType.icon, documentType.name);

		// Map action to document type
		pSignalMapper->setMapping(pAction, documentType.name);
		checkedConnect(pAction, SIGNAL(triggered()), pSignalMapper, SLOT(map()));
	}

	checkedConnect(pSignalMapper, SIGNAL(mapped(const QString&)), &mainWindow, SLOT(openDocument(const QString&)));

	// Add 'Open As'-menu to toolbar
	QToolButton *pOpenButton = qobject_cast<QToolButton*>(
		mainWindow.widgets().fileToolBar->widgetForAction(mainWindow.widgets().actionOpen) );
	pOpenButton->setMenu(pOpenMenu);
	pOpenButton->setPopupMode(QToolButton::MenuButtonPopup);
}

/// Adds a document window.
bool addDocumentWindow(const Ui::MainWindow &mainWindow, QWidget *pView, AbstractDocument *pDocument, Mode *pDocumentMode, const DocumentType &documentType, Editor *pEditor)
{
	// Create document window
	std::auto_ptr<MdiDocumentWindow> pWindow( new MdiDocumentWindow(pDocument) );
	pWindow->setAttribute(Qt::WA_DeleteOnClose);

	// Create default view, if none given
	if (!pView)
	{
		try
		{
			pView = documentType.pFactory->createDocumentView(pDocument, pDocumentMode, pEditor);
		}
		catch (...)
		{
			exceptionToMessageBox(
					MainWindow::tr("Error creating document view"),
					MainWindow::tr("Error while creating document view, you might need to adjust your settings.")
				);
			return false;
		}
	}
	pWindow->setWidget(pView);

	// ORDER: Add subwindow after FULL CONSTRUCTION (activation signals ...)
	mainWindow.mdiArea->addSubWindow(pWindow.get());
	pWindow.release()->show();

	return true;
}

/// Adds a document.
Mode* maybeAddDocumentMode(MainWindow::document_mode_map &modes, Mode &modeStack,
	AbstractDocument *pDocument, const DocumentType &documentType,
	MainWindow &mainWindow, Editor *pEditor)
{
	MainWindow::document_mode_map::const_iterator itMode = modes.constFind(pDocument);

	if (itMode == modes.constEnd())
	{
		std::auto_ptr<Mode> pMode( documentType.pFactory->createDocumentMode(pDocument, pEditor, &modeStack) );

		// Keep track of document life time
		checkedConnect(pDocument, SIGNAL(documentClosing(AbstractDocument*, bool)),
			&mainWindow, SLOT(documentClosing(AbstractDocument*)));

		modes[pDocument] = pMode.get();

		return pMode.release();
	}
	else
		return *itMode;
}

} // namespace

// Constructor.
MainWindow::MainWindow(Editor *pEditor, QWidget *pParent, Qt::WindowFlags flags)
	: QMainWindow(pParent, flags),
	m_pEditor( LEAN_ASSERT_NOT_NULL(pEditor) ),
	m_pModeStack( new Mode(this) ),
	m_pDocument()
{
	ui.setupUi(this);

/*	QPalette mainPalette = this->palette();
	mainPalette.setColor(QPalette::Window, mainPalette.window().color().darker(110));
	this->setPalette(mainPalette);
*/
	// TODO: This is TEST CODE!
//	if (false)
	{
		// Retrieve center widget
		QScopedPointer<QWidget> centerWidget( this->centralWidget() );
		centerWidget->setParent(nullptr);

		// Wrap
		m_dock = new DockContainer(this);
		this->setCentralWidget(m_dock);
		m_dock->setCentralWidget(centerWidget.take());
		this->setCentralWidget(m_dock);

		QColor b = m_dock->palette().window().color();
		QColor f = b;
		b.setRed( b.red() / 5 );
//		b.setGreen( b.green() / 4 );
		b.setGreen( b.green() / 5 );
//		b.setBlue( b.blue() / 3 );
		b.setBlue( b.blue() / 5 );
		f.setRed( 255 - (255 - f.red()) / 3 );
		f.setGreen( f.red() * 15 / 16 );
		f.setBlue( f.red() * 4 / 5 );
		m_dock->setStyleSheet(
				QString(
				"#%2::handle, #%3 > * { background-color:%4; }\n"
					"#%1 { border:5px solid %4; }\n"
					"#DockGroup > #TitleBar { background-color: %5; }\n"
					"#DockGroup > #TabBar { background-color: %4; color:white; }\n"
					"#DockGroup > #TabBar > * { /* background-color: %4; */ color:white; }\n"
				).arg(m_dock->objectName(), m_dock->nestedSplitterName(), m_dock->centerName(), b.name(), f.name())
			);
	}

	m_pInfoLabel = new QLabel(ui.statusBar);
	m_pInfoLabel->setMinimumWidth(18);
	ui.statusBar->addPermanentWidget(m_pInfoLabel);

	m_pConsole = addConsoleWidget(*this, m_pEditor);
	new ConsoleWidgetLogBinder(m_pConsole, &lean::error_log(), m_pConsole);
	new ConsoleWidgetLogBinder(m_pConsole, &lean::info_log(), m_pConsole);

	checkedConnect(ui.actionNew, SIGNAL(triggered()), this, SLOT(newDocument()));
	checkedConnect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(openDocument()));
	addDocumentTypes(*this, *m_pEditor);

	mainWindowPlugins().initializePlugins(this);

	checkedConnect(ui.mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(documentActivated(QMdiSubWindow*)));

	LEAN_LOG("Starting up");

	m_pModeStack->enter();
	
	// WORKAROUND: Restore broken for hidden main window
//	show();
	restoreWindow(*this, *m_dock, *m_pEditor);
}

// Destructor.
MainWindow::~MainWindow()
{
	m_pModeStack->exit();

	LEAN_LOG("Shutting down");

	mainWindowPlugins().finalizePlugins(this);
}

// Sets editing information.
void MainWindow::setEditingInfo(const QString &info)
{
	QString windowTitle = "breezEd";

	if (!info.isEmpty())
		windowTitle += " [" + info + "]";

	setWindowTitle(windowTitle);
}

// Adds the given dock widget.
void MainWindow::addDockWidgetTabified(Qt::DockWidgetArea area, QDockWidget *dockwidget)
{
//	if (!this->restoreDockWidget(dockwidget))
	{
		QList<QDockWidget*> docks = this->findChildren<QDockWidget*>();
		QDockWidget *dockTab = nullptr;

		Q_FOREACH (QDockWidget *dock, docks)
			if (this->dockWidgetArea(dock) & area)
			{
				dockTab = dock;
				break;
			}

		this->addDockWidget(area, dockwidget);
		this->tabifyDockWidget(dockTab, dockwidget);
	}
}

// Retrieves the dock widget parent to the given widget, potentially overlapping the given dock widget.
QDockWidget* MainWindow::overlappingDockWidget(QDockWidget *dockwidget, QWidget *potentiallyOverlapping) const
{
	if (potentiallyOverlapping)
		Q_FOREACH(QDockWidget *overlapping, this->tabifiedDockWidgets(dockwidget))
			if (isChildOf(potentiallyOverlapping, overlapping))
				return overlapping;

	return nullptr;
}

// Opens the new document dialog.
void MainWindow::newDocument()
{
	NewDocumentDialog newDocumentDialog(m_pEditor, this);

	// Open dialog
	while (NewDocumentDialog::Accepted == newDocumentDialog.exec())
	{
		const DocumentType &documentType = newDocumentDialog.documentType();

		// Check document type
		if (documentType.valid())
		{
			QString documentFile = documentType.pFactory->fileFromDir(newDocumentDialog.documentName(), newDocumentDialog.documentPath());
			bool bCreate = true;

			if (QFile::exists(documentFile))
			{
				QMessageBox::StandardButton result = QMessageBox::question( nullptr,
						tr("Overwrite file?"),
						tr("The document file '%1' already exists. Overwrite?").arg(documentFile),
						QMessageBox::Yes | QMessageBox::Cancel
					);

				// Prompt to overwrite
				if (result != QMessageBox::Yes)
					bCreate = false;
			}

			if (bCreate)
			{
				try
				{
					// Create document
					DocumentReference<AbstractDocument> pDocument = documentType.pFactory->createDocument(
							newDocumentDialog.documentName(), documentFile, m_pEditor, this
						);
					
					addDocument(pDocument, nullptr);
				}
				catch (...)
				{
					exceptionToMessageBox(
							MainWindow::tr("Error creating document"),
							MainWindow::tr("An unexpected error occurred while creating '%1' document '%2'.").arg(documentType.name).arg(documentFile)
						);
				}

				return;
			}
		}
		else
			QMessageBox::critical( nullptr,
					tr("Invalid document type"),
					tr("Unknown document type '%1'.").arg(documentType.name)
				);
	}
}

// Opens a document.
bool MainWindow::openDocument()
{
	QString location = m_pEditor->settings()->value("mainWindow/openPath", QDir::currentPath()).toString();

	// Open file browser dialog
	QString openFile = QFileDialog::getOpenFileName(this, tr("Select a file to open."), location);

	if (!openFile.isEmpty())
	{
		m_pEditor->settings()->setValue("mainWindow/openPath", QFileInfo(openFile).absolutePath());

		QList<DocumentType> documentTypes = m_pEditor->documentManager()->documentTypes();
		QList<DocumentType> matchingTypes, anyTypes;

		Q_FOREACH (const DocumentType &documentType, documentTypes)
			// Find matching document types
			switch (documentType.pFactory->matchFile(openFile))
			{
			case FileMatch::Match:
				matchingTypes.push_back(documentType);
				break;
			case FileMatch::Any:
				anyTypes.push_back(documentType);
				break;
			}

		// Check if unambiguous
		if (matchingTypes.size() != 1)
			matchingTypes.append(anyTypes);

		if (!matchingTypes.empty())
		{
			const DocumentType *selectedType = &matchingTypes.first();

			// Resolve ambiguities
			if (matchingTypes.size() > 1)
			{
				QStringList matchingTypeNames;

				// Build list of matching document type names
				Q_FOREACH (const DocumentType &documentType, matchingTypes)
					matchingTypeNames.push_back(documentType.name);

				// Select a matching document type
				bool ok = false;
				QString selectedTypeName = QInputDialog::getItem(this,
					tr("Select a document type."),
					tr("Select a document type for '%1':").arg(openFile),
					matchingTypeNames, 0,
					false, &ok);

				selectedType = (ok)
					? &matchingTypes.at(matchingTypeNames.indexOf(selectedTypeName))
					: nullptr;
			}

			// Check if cancelled during ambiguity resolution
			if (selectedType)
			{
				try
				{
					// Open document
					DocumentReference<AbstractDocument> pDocument = selectedType->pFactory->openDocument(openFile, m_pEditor, this);
				
					if (pDocument)
						return addDocument(pDocument, nullptr);
					else
						QMessageBox::critical( nullptr,
							tr("Error while opening"),
							tr("Could not open '%1' file '%2'.").arg(selectedType->name).arg(openFile)
						);
				}
				catch (...)
				{
					exceptionToMessageBox(
							MainWindow::tr("Error opening document"),
							MainWindow::tr("An unexpected error occurred while opening '%1' document '%2'.").arg(selectedType->name).arg(openFile)
						);
				}
			}
		}
		else
			QMessageBox::critical( nullptr,
					tr("Unknown document type"),
					tr("No matching document type found for '%1'.").arg(openFile)
				);
	}

	return false;
}

// Opens a document of the given type.
bool MainWindow::openDocument(const QString &documentTypeName)
{
	QString location = m_pEditor->settings()->value("mainWindow/openPath/" + documentTypeName, QDir::currentPath()).toString();

	// Open file browser dialog
	QString openFile = QFileDialog::getOpenFileName(m_pEditor->mainWindow(), tr("Select a '%1' file to open.").arg(documentTypeName), location);

	if (!openFile.isEmpty())
	{
		m_pEditor->settings()->setValue("mainWindow/openPath/" + documentTypeName, QFileInfo(openFile).absolutePath());

		const DocumentType &documentType = m_pEditor->documentManager()->documentType(documentTypeName);

		if (documentType.valid())
		{
			try
			{
				// Open document
				DocumentReference<AbstractDocument> pDocument = documentType.pFactory->openDocument(openFile, m_pEditor, this);

				if (pDocument)
					return addDocument(pDocument, nullptr);
				else
					QMessageBox::critical( nullptr,
						tr("Error opening document"),
						tr("Could not open '%1' file '%2'.").arg(documentTypeName).arg(openFile)
					);
			}
			catch (...)
			{
				exceptionToMessageBox(
						MainWindow::tr("Error opening document"),
						MainWindow::tr("An unexpected error occurred while opening '%1' document '%2'.").arg(documentTypeName).arg(openFile)
					);
			}
		}
		else
			QMessageBox::critical( nullptr,
					tr("Invalid document type"),
					tr("Unknown document type '%1'.").arg(documentTypeName)
				);
	}

	return false;
}

// Adds the given document.
bool MainWindow::addDocument(AbstractDocument *pDocument, QWidget *pView)
{
	LEAN_ASSERT_NOT_NULL(pDocument);

	const DocumentType &documentType = m_pEditor->documentManager()->documentType(pDocument->type());

	if (documentType.valid())
	{
		// Create document mode
		Mode *pMode = maybeAddDocumentMode(m_documentModes, *m_pModeStack, pDocument, documentType, *this, m_pEditor);

		// Create document window
		return addDocumentWindow(ui, pView, pDocument, pMode, documentType, m_pEditor);
	}
	else
		return false;
}

// Intercepts the close event.
void MainWindow::closeEvent(QCloseEvent *pEvent)
{
	QList<QMdiSubWindow*> documentWindows = ui.mdiArea->subWindowList();

	Q_FOREACH (QMdiSubWindow *pDocumentWindow, documentWindows)
	{
		// Try to close document window
		if (!pDocumentWindow->close())
		{
			// Document window refuses to close, cancel
			pEvent->ignore();
			return;
		}
	}

	// Save layout changes
	saveWindow(*this, *m_dock, *m_pEditor);

	// Close main window
	QMainWindow::closeEvent(pEvent);
}

// Keeps track of document selections.
void MainWindow::documentActivated(QMdiSubWindow *pWindow)
{
	MdiDocumentWindow *pDocumentWindow = qobject_cast<MdiDocumentWindow*>(pWindow);

	if (pDocumentWindow)
	{
		AbstractView *pView = qobject_cast<AbstractView*>(pWindow->widget());

		// Activate view
		if (pView)
			pView->activate();

		document_mode_map::const_iterator itDocumentMode = m_documentModes.constFind(pDocumentWindow->document());

		// Enter document mode
		if (itDocumentMode != m_documentModes.constEnd())
			(*itDocumentMode)->enter();

		AbstractDocument *pDocument = pDocumentWindow->document();

		if (pDocument != m_pDocument)
		{
			m_pDocument = pDocument;
			Q_EMIT documentChanged(m_pDocument);
		}
	}
}

// Keeps track of document life time.
void MainWindow::documentClosing(AbstractDocument *pDocument)
{
	if (pDocument == m_pDocument)
	{
		m_pDocument = nullptr;
		Q_EMIT documentChanged(nullptr);
	}

	Q_EMIT documentClosed(pDocument);

	document_mode_map::iterator itDocumentMode = m_documentModes.find(pDocument);

	if (itDocumentMode != m_documentModes.end())
	{
		Mode *pDocumentMode = *itDocumentMode;
		m_documentModes.erase(itDocumentMode);
		delete pDocumentMode;
	}
}

namespace
{

/// Casts QList<Src> to QList<Dest>.
template <class Dest, class Src>
QList<Dest> list_cast(const QList<Src> &list)
{
	QList<Dest> dest;

	Q_FOREACH (const Src &src, list)
		dest.append( static_cast<Dest>(src) );

	return dest;
}

} // namespace

// Shows the windows dialog.
void MainWindow::showWindowsDialog()
{
	WindowsDialog windowsDialog( list_cast<QWidget*>(ui.mdiArea->subWindowList()), this );
	windowsDialog.exec();
}

// Tiles all subwindows.
void MainWindow::tileWindows()
{
	struct SubWindowOrder
	{
		bool operator ()(QWidget *pLeft, QWidget *pRight)
		{
			QPoint pos1 = pLeft->pos(), pos2 = pRight->pos();
			QSize size1 = pLeft->size(), size2 = pRight->size();

			// Vertical sorting
			if( pos1.y() + size1.height() / 2 > pos2.y() + size2.height() )
				return true;
			else if( pos2.y() + size2.height() / 2 > pos1.y() + size1.height() )
				return false;

			// Horizontal sorting
			return pos1.x() > pos2.x();
		}
	};

	// Sort windows according to their position
	QList<QMdiSubWindow*> subWindows = ui.mdiArea->subWindowList();
	qSort(subWindows.begin(), subWindows.end(), SubWindowOrder());

	QMdiArea::WindowOrder previousActivationOrder = ui.mdiArea->activationOrder();

	// Activate windows in order
	Q_FOREACH (QMdiSubWindow *pWindow, subWindows)
		ui.mdiArea->setActiveSubWindow(pWindow);

	// Tile windows arrocding to activation history
	ui.mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
	ui.mdiArea->tileSubWindows();

	// Reset activation order
	ui.mdiArea->setActivationOrder(previousActivationOrder);
}

// Gets the main window plugin manager.
PluginManager<MainWindow*>& mainWindowPlugins()
{
	static PluginManager<MainWindow*> pluginManager;
	return pluginManager;
}
