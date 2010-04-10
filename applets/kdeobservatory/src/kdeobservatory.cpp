/*************************************************************************
 * Copyright 2009 Sandro Andrade sandroandrade@kde.org                   *
 *                                                                       *
 * This program is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU General Public License as        *
 * published by the Free Software Foundation; either version 2 of        *
 * the License or (at your option) version 3 or any later version        *
 * accepted by the membership of KDE e.V. (or its successor approved     *
 * by the membership of KDE e.V.), which shall act as a proxy            *
 * defined in Section 14 of version 3 of the license.                    *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 * ***********************************************************************/

#include "kdeobservatory.h"

#include <QTimer>
#include <QTimeLine>
#include <QGraphicsScene>
#include <QListWidgetItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsLinearLayout>
#include <QGraphicsItemAnimation>

#include <KConfig>
#include <KConfigDialog>
#include <KGlobalSettings>

#include <Plasma/Label>
#include <Plasma/Meter>
#include <Plasma/DataEngine>
#include <Plasma/PushButton>

#include "krazycollector.h"
#include "commitcollector.h"

#include "krazyreportview.h"
#include "topdevelopersview.h"
#include "commithistoryview.h"
#include "topactiveprojectsview.h"

#include "kdeobservatorypresets.h"
#include "kdeobservatorydatabase.h"
#include "kdeobservatoryconfigviews.h"
#include "kdeobservatoryconfiggeneral.h"
#include "kdeobservatoryconfigprojects.h"

K_EXPORT_PLASMA_APPLET(kdeobservatory, KdeObservatory)

KdeObservatory::KdeObservatory(QObject *parent, const QVariantList &args)
: Plasma::PopupApplet(parent, args),
  m_mainContainer(0),
  m_currentView(0),
  m_transitionTimer(0)
{
    setBackgroundHints(DefaultBackground);
    setHasConfigurationInterface(true);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    resize(300, 200);

    m_collectors["Commit Collector"] = new CommitCollector(this);
    m_collectors["Krazy Collector"] = new KrazyCollector(m_krazyReportViewProjects, m_projects, this);

    connect(m_collectors["Commit Collector"], SIGNAL(collectFinished()), this, SLOT(collectFinished()));
    connect(m_collectors["Krazy Collector"], SIGNAL(collectFinished()), this, SLOT(collectFinished()));
    KdeObservatoryPresets::init();
}

KdeObservatory::~KdeObservatory()
{
    if (!hasFailedToLaunch())
    {
        delete m_viewProviders[i18n("Top Active Projects")];
        delete m_viewProviders[i18n("Top Developers")];
        delete m_viewProviders[i18n("Commit History")];
        delete m_viewProviders[i18n("Krazy Report")];
        KdeObservatoryDatabase::self()->~KdeObservatoryDatabase();
    }
}

void KdeObservatory::init()
{
    loadConfig();
    graphicsWidget();
    createTimers();
    createViewProviders();
    runCollectors();
    setPopupIcon(KIcon("kdeobservatory"));
    Plasma::DataEngine *engine = dataEngine("kdecommits");
    if (!engine) kDebug() << "Nao leu engine"; else kDebug() << "Leu o engine !";
    Plasma::DataEngine::Data data = engine->query("top-projects");
    kDebug() << "Top project names  : " << data["projects"].toStringList();
    kDebug() << "Top project commit#: " << data["commits"].toStringList();

    Plasma::DataEngine::Data data2 = engine->query("top-developers/plasma");
    kDebug() << "Top plasma developer names  : " << data2["developers"].toStringList();
    kDebug() << "Top plasma developer commit#: " << data2["commits"].toStringList();

    Plasma::DataEngine::Data data3 = engine->query("krazy-report/plasma");
    QStringList list1 = data3["types"].toStringList();
    QList<QVariant> list2 = data3["errorLists"].toList();
    
    for (int i = 0; i < list1.size(); ++i)
    {
        QStringList list3 = list2.at(i).toStringList();
        kDebug() << "Error of type " << list1.at(i) << list3;
    }
}

QGraphicsWidget *KdeObservatory::graphicsWidget()
{
    if (!m_mainContainer)
    {
        CommitCollector *commitCollector = qobject_cast<CommitCollector *>(m_collectors["Commit Collector"]);

        m_mainContainer = new QGraphicsWidget(this);
        m_mainContainer->installEventFilter(this);

        m_viewContainer = new QGraphicsWidget(m_mainContainer);
        m_viewContainer->setAcceptHoverEvents(true);
        m_viewContainer->setHandlesChildEvents(true);
        m_viewContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_viewContainer->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

        m_right = new Plasma::PushButton(m_mainContainer);
        m_right->setIcon(KIcon("go-next-view"));
        m_right->setToolTip(i18n("Go to previous view"));
        m_right->setMaximumSize(22, 22);
        connect(m_right, SIGNAL(clicked()), this, SLOT(moveViewRightClicked()));

        m_left = new Plasma::PushButton(m_mainContainer);
        m_left->setIcon(KIcon("go-previous-view"));
        m_left->setToolTip(i18n("Go to next view"));
        m_left->setMaximumSize(22, 22);
        connect(m_left, SIGNAL(clicked()), this, SLOT(moveViewLeftClicked()));

        m_collectorProgress = new Plasma::Meter(m_mainContainer);
        m_collectorProgress->setMeterType(Plasma::Meter::BarMeterHorizontal);
        m_collectorProgress->setMaximumHeight(22);
        m_collectorProgress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(commitCollector, SIGNAL(progressMaximum(int)), m_collectorProgress, SLOT(setMaximum(int)));
        connect(commitCollector, SIGNAL(progressValue(int)), m_collectorProgress, SLOT(setValue(int)));
        m_collectorProgress->hide();

        m_updateLabel = new Plasma::Label(m_mainContainer);
        m_updateLabel->setText("");
        m_updateLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_updateLabel->setFont(KGlobalSettings::smallestReadableFont());
        m_updateLabel->setAlignment(Qt::AlignCenter);

        m_horizontalLayout = new QGraphicsLinearLayout(Qt::Horizontal);
        m_horizontalLayout->addItem(m_left);
        m_horizontalLayout->addItem(m_updateLabel);
        m_horizontalLayout->addItem(m_right);
        m_horizontalLayout->setMaximumHeight(22);

        QGraphicsLinearLayout *verticalLayout = new QGraphicsLinearLayout(Qt::Vertical);
        verticalLayout->addItem(m_viewContainer);
        verticalLayout->addItem(m_horizontalLayout);

        m_mainContainer->setLayout(verticalLayout);
        m_mainContainer->setPreferredSize(300, 200);
        m_mainContainer->setMinimumSize(300, 200);
    }
    return m_mainContainer;
}

bool KdeObservatory::eventFilter(QObject *receiver, QEvent *event)
{
    if (!m_viewProviders.isEmpty() &&
        dynamic_cast<QGraphicsWidget *>(receiver) == m_mainContainer &&
        event->type() == QEvent::GraphicsSceneResize)
    {
        prepareUpdateViews();
        updateViews();
    }
    return Plasma::PopupApplet::eventFilter(receiver, event);
}

bool KdeObservatory::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (event->type() == QEvent::GraphicsSceneHoverEnter && m_enableAutoViewChange)
        m_viewTransitionTimer->stop();
    if (event->type() == QEvent::GraphicsSceneHoverLeave && m_enableAutoViewChange)
        m_viewTransitionTimer->start();
    return false;
}

void KdeObservatory::createConfigurationInterface(KConfigDialog *parent)
{
    m_configGeneral = new KdeObservatoryConfigGeneral(parent);
    parent->addPage(m_configGeneral, i18nc("Global configuration options", "General"), "applications-development");

    m_configProjects = new KdeObservatoryConfigProjects(parent);
    parent->addPage(m_configProjects, i18n("Projects"), "project-development");

    m_configViews = new KdeObservatoryConfigViews(parent);
    m_configViews->m_projects = m_projects;
    m_configViews->m_projectsInView[i18n("Top Active Projects")] = m_topActiveProjectsViewProjects;
    m_configViews->m_projectsInView[i18n("Top Developers")] = m_topDevelopersViewProjects;
    m_configViews->m_projectsInView[i18n("Commit History")] = m_commitHistoryViewProjects;
    m_configViews->m_projectsInView[i18n("Krazy Report")] = m_krazyReportViewProjects;
    m_configViews->updateView(i18n("Top Active Projects"));
    parent->addPage(m_configViews, i18n("Views"), "view-presentation");

    connect(m_configProjects, SIGNAL(projectAdded(const QString &, const QString &)),
            m_configViews, SLOT(projectAdded(const QString &, const QString &)));
    connect(m_configProjects, SIGNAL(projectRemoved(const QString &)),
            m_configViews, SLOT(projectRemoved(const QString &)));

    // Config - General
    m_configGeneral->commitExtent->setValue(m_commitExtent);
    m_configGeneral->synchronizationDelay->setTime(QTime(m_synchronizationDelay/3600, (m_synchronizationDelay/60)%60, m_synchronizationDelay%60));
    m_configGeneral->enableAutoViewChange->setChecked(m_enableAutoViewChange);
    m_configGeneral->viewsDelay->setTime(QTime(m_viewsDelay/3600, (m_viewsDelay/60)%60, m_viewsDelay%60));

    int viewsCount = m_activeViews.count();
    for (int i = 0; i < viewsCount; ++i)
    {
        QListWidgetItem * item = m_configViews->activeViews->findItems(m_activeViews.at(i).first, Qt::MatchFixedString).first();
        item->setCheckState(m_activeViews.at(i).second == true ? Qt::Checked:Qt::Unchecked);
        m_configViews->activeViews->takeItem(m_configViews->activeViews->row(item));
        m_configViews->activeViews->insertItem(i, item);
    }

    // Config - Projects
    QMapIterator<QString, Project> i(m_projects);
    while (i.hasNext())
    {
        i.next();
        Project project = i.value();
        m_configProjects->createTableWidgetItem(i.key(), project.commitSubject, project.krazyReport, project.krazyFilePrefix, project.icon);
        m_configProjects->projects->setCurrentCell(0, 0);
    }

    m_configProjects->projects->setCurrentItem(m_configProjects->projects->item(0, 0));
    m_configProjects->projects->resizeColumnsToContents();
    m_configProjects->projects->horizontalHeader()->setStretchLastSection(true);

    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
}

void KdeObservatory::configAccepted()
{
    prepareUpdateViews();

    CommitCollector *commitCollector = qobject_cast<CommitCollector *>(m_collectors["Commit Collector"]);

    bool commitExtentChanged = false;
    if (m_configGeneral->commitExtent->value() != m_commitExtent)
    {
        if (m_configGeneral->commitExtent->value() > m_commitExtent)
        {
            commitCollector->setFullUpdate(true);
        }
        else
        {
            commitExtentChanged = true;
        }
    }

    // General properties
    m_commitExtent = m_configGeneral->commitExtent->value();
    QTime synchronizationDelay = m_configGeneral->synchronizationDelay->time();
    m_synchronizationDelay = synchronizationDelay.second() + synchronizationDelay.minute()*60 + synchronizationDelay.hour()*3600;
    m_enableAutoViewChange = (m_configGeneral->enableAutoViewChange->checkState() == Qt::Checked) ? true:false;
    QTime viewsDelay = m_configGeneral->viewsDelay->time();
    m_viewsDelay = viewsDelay.second() + viewsDelay.minute()*60 + viewsDelay.hour()*3600;

    m_activeViews.clear();
    QStringList viewNames;
    QList<bool> viewActives;

    for (int i = 0; i < m_configViews->activeViews->count(); ++i)
    {
        QListWidgetItem *item = m_configViews->activeViews->item(i);
        QString viewName = item->text();
        bool viewActive = (item->checkState() == Qt::Checked) ? true:false;
        m_activeViews << QPair<QString, bool>(viewName, viewActive);
        viewNames << viewName;
        viewActives << viewActive;
    }

    commitCollector->setExtent(m_commitExtent);
    m_viewTransitionTimer->setInterval(m_viewsDelay * 1000);
    m_synchronizationTimer->setInterval(m_synchronizationDelay * 1000);

    // Projects properties
    QStringList projectNames;
    QStringList projectCommitSubjects;
    QStringList projectKrazyReports;
    QStringList projectKrazyFilePrefix;
    QStringList projectIcons;

    m_projects.clear();

    int projectsCount = m_configProjects->projects->rowCount();
    for (int i = 0; i < projectsCount; ++i)
    {
        Project project;
        project.commitSubject = m_configProjects->projects->item(i, 1)->text();
        project.krazyReport = m_configProjects->projects->item(i, 2)->text();
        project.krazyFilePrefix = m_configProjects->projects->item(i, 3)->text();
        project.icon = m_configProjects->projects->item(i, 0)->data(Qt::UserRole).value<QString>();
        m_projects[m_configProjects->projects->item(i, 0)->text()] = project;
        projectNames << m_configProjects->projects->item(i, 0)->text();
        projectCommitSubjects << project.commitSubject;
        projectKrazyReports << project.krazyReport;
        projectKrazyFilePrefix << project.krazyFilePrefix;
        projectIcons << project.icon;
    }

    m_configViews->on_views_currentIndexChanged(i18n("Top Active Projects"));
    m_topActiveProjectsViewProjects = m_configViews->m_projectsInView[i18n("Top Active Projects")];
    m_topDevelopersViewProjects = m_configViews->m_projectsInView[i18n("Top Developers")];
    m_commitHistoryViewProjects = m_configViews->m_projectsInView[i18n("Commit History")];
    m_krazyReportViewProjects = m_configViews->m_projectsInView[i18n("Krazy Report")];

    saveConfig();

    if (commitCollector->fullUpdate() || commitExtentChanged)
        runCollectors();
    else
        updateViews();
}

void KdeObservatory::collectFinished()
{
    ++m_collectorsFinished;

    if (m_collectorsFinished == m_collectors.count())
    {
        prepareUpdateViews();

        setBusy(false);
        m_updateLabel->setText(i18n("Last update: ") + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss"));
        m_collectorProgress->hide();
        m_horizontalLayout->removeItem(m_collectorProgress);
        m_horizontalLayout->insertItem(1, m_updateLabel);
        m_updateLabel->show();
        m_right->setEnabled(true);
        m_left->setEnabled(true);

        updateViews();

        m_configGroup.writeEntry("commitsRead", (qobject_cast<CommitCollector *>(m_collectors["Commit Collector"]))->commitsRead());
        m_configGroup.writeEntry("lastArchiveRead", (qobject_cast<CommitCollector *>(m_collectors["Commit Collector"]))->lastArchiveRead());
        m_configGroup.writeEntry("lastKrazyCollect", (qobject_cast<KrazyCollector *>(m_collectors["Krazy Collector"]))->lastCollect());

        m_synchronizationTimer->start();
    }
}

void KdeObservatory::moveViewRight()
{
    switchViews(1);
}

void KdeObservatory::moveViewLeft()
{
    switchViews(-1);
}

void KdeObservatory::moveViewRightClicked()
{
    m_viewTransitionTimer->stop();
    moveViewRight();
    // Causes a short delay (10s) to next transition
    if (m_enableAutoViewChange)
        QTimer::singleShot(10000, m_viewTransitionTimer, SLOT(start()));
}

void KdeObservatory::moveViewLeftClicked()
{
    m_viewTransitionTimer->stop();
    moveViewLeft();
    // Causes a short delay (10s) to next transition
    if (m_enableAutoViewChange)
        QTimer::singleShot(10000, m_viewTransitionTimer, SLOT(start()));
}

void KdeObservatory::switchViews(int delta)
{
    if (m_views.count() > 0)
    {
        int previousView = m_currentView;
        int newView = m_currentView + delta;
        m_currentView = (newView >= 0) ? (newView % m_views.count()):(m_views.count() + newView);

        QGraphicsWidget *previousViewWidget = m_views.at(previousView);
        QGraphicsWidget *currentViewWidget = m_views.at(m_currentView);
        currentViewWidget->setPos(currentViewWidget->geometry().width(), 0);
        currentViewWidget->show();

        m_transitionTimer = new QTimeLine(500, this);
        m_transitionTimer->setFrameRange(0, 1);
        m_transitionTimer->setCurveShape(QTimeLine::EaseOutCurve);

        QGraphicsItemAnimation *animationPrevious = new QGraphicsItemAnimation(this);
        animationPrevious->setItem(previousViewWidget);
        animationPrevious->setTimeLine(m_transitionTimer);
        animationPrevious->setPosAt(0, QPointF(0, 0));
        animationPrevious->setPosAt(1, -delta*QPointF(previousViewWidget->geometry().width(), 0));

        QGraphicsItemAnimation *animationNew = new QGraphicsItemAnimation(this);
        animationNew->setItem(currentViewWidget);
        animationNew->setTimeLine(m_transitionTimer);
        animationNew->setPosAt(0, delta*QPointF(currentViewWidget->geometry().width(), 0));
        animationNew->setPosAt(1, QPointF(0, 0));

        m_transitionTimer->start();
    }
}

void KdeObservatory::runCollectors()
{
    m_right->setEnabled(false);
    m_left->setEnabled(false);

    setBusy(true);
    m_updateLabel->hide();
    m_horizontalLayout->removeItem(m_updateLabel);
    m_horizontalLayout->insertItem(1, m_collectorProgress);
    m_collectorProgress->show();

    m_collectorsFinished = 0;
    m_lastViewCount = m_views.count();
    m_synchronizationTimer->stop();

    foreach (ICollector *collector, m_collectors)
        collector->run();
}

void KdeObservatory::prepareUpdateViews()
{
    m_viewTransitionTimer->stop();
    m_synchronizationTimer->stop();
    if (m_transitionTimer)
        m_transitionTimer->stop();

    foreach(QGraphicsWidget *widget, m_views)
        widget->hide();
}

void KdeObservatory::updateViews()
{
    m_views.clear();
    int count = m_activeViews.count();
    for (int i = 0; i < count; ++i)
    {
        const QPair<QString, bool> &pair = m_activeViews.at(i);
        const QString &view = pair.first;
        if (pair.second && m_viewProviders.value(view))
        {
            m_viewProviders[view]->updateViews();
            m_views.append(m_viewProviders[view]->views());
        }
    }

    if (m_views.count() > 0)
    {
        if (m_views.count() != m_lastViewCount)
            m_currentView = m_views.count()-1;
        moveViewRight();
        if (m_enableAutoViewChange)
            m_viewTransitionTimer->start();
    }
    m_synchronizationTimer->start();
}

void KdeObservatory::loadConfig()
{
    m_configGroup = config();

    CommitCollector *commitCollector = qobject_cast<CommitCollector *>(m_collectors["Commit Collector"]);
    KrazyCollector  *krazyCollector  = qobject_cast<KrazyCollector * >(m_collectors["Krazy Collector"]);

    commitCollector->setCommitsRead(m_configGroup.readEntry("commitsRead", 0));
    commitCollector->setLastArchiveRead(m_configGroup.readEntry("lastArchiveRead", QString()));
    krazyCollector->setLastCollect(m_configGroup.readEntry("lastKrazyCollect", QString()));

    if (commitCollector->commitsRead() == 0)
        commitCollector->setFullUpdate(true);

    // Config - General
    m_commitExtent = m_configGroup.readEntry("commitExtent", 7);
    m_synchronizationDelay = m_configGroup.readEntry("synchronizationDelay", 300);
    m_enableAutoViewChange = m_configGroup.readEntry("enableAutoViewChange", true);
    m_viewsDelay = m_configGroup.readEntry("viewsDelay", 5);

    QStringList viewNames = m_configGroup.readEntry("viewNames", KdeObservatoryPresets::viewsPreset());
    QList<bool> viewActives = m_configGroup.readEntry("viewActives", KdeObservatoryPresets::viewsActivePreset());

    m_activeViews.clear();
    int viewsCount = viewNames.count();
    for (int i = 0; i < viewsCount; ++i)
        m_activeViews.append(QPair<QString, bool>(viewNames.at(i), viewActives.at(i)));

    // Config - Projects
    QStringList projectNames = m_configGroup.readEntry("projectNames", KdeObservatoryPresets::preset(KdeObservatoryPresets::ProjectName));
    QStringList projectCommitSubjects = m_configGroup.readEntry("projectCommitSubjects", KdeObservatoryPresets::preset(KdeObservatoryPresets::CommitSubject));
    QStringList projectKrazyReports = m_configGroup.readEntry("projectKrazyReports", KdeObservatoryPresets::preset(KdeObservatoryPresets::KrazyReport));
    QStringList projectKrazyFilePrefix = m_configGroup.readEntry("projectKrazyFilePrefix", KdeObservatoryPresets::preset(KdeObservatoryPresets::KrazyFilePrefix));
    QStringList projectIcons = m_configGroup.readEntry("projectIcons", KdeObservatoryPresets::preset(KdeObservatoryPresets::Icon));

    m_projects.clear();
    int projectsCount = projectNames.count();
    for (int i = 0; i < projectsCount; ++i)
    {
        Project project;
        project.commitSubject = projectCommitSubjects.at(i);
        project.krazyReport = projectKrazyReports.at(i);
        project.krazyFilePrefix = projectKrazyFilePrefix.at(i);
        project.icon = projectIcons.at(i);
        m_projects[projectNames.at(i)] = project;
    }

    // Config - Top Active Projects
    QStringList topActiveProjectsViewNames = m_configGroup.readEntry("topActiveProjectsViewNames", KdeObservatoryPresets::preset(KdeObservatoryPresets::ProjectName));
    QList<bool> topActiveProjectsViewActives = m_configGroup.readEntry("topActiveProjectsViewActives", KdeObservatoryPresets::automaticallyInViews());

    m_topActiveProjectsViewProjects.clear();
    int topActiveProjectsViewsCount = topActiveProjectsViewNames.count();
    for (int i = 0; i < topActiveProjectsViewsCount; ++i)
        m_topActiveProjectsViewProjects[topActiveProjectsViewNames.at(i)] = topActiveProjectsViewActives.at(i);

    // Config - Top Developers
    QStringList topDevelopersViewNames = m_configGroup.readEntry("topDevelopersViewNames", KdeObservatoryPresets::preset(KdeObservatoryPresets::ProjectName));
    QList<bool> topDevelopersViewActives = m_configGroup.readEntry("topDevelopersViewActives", KdeObservatoryPresets::automaticallyInViews());

    m_topDevelopersViewProjects.clear();
    int topDevelopersViewsCount = topDevelopersViewNames.count();
    for (int i = 0; i < topDevelopersViewsCount; ++i)
        m_topDevelopersViewProjects[topDevelopersViewNames.at(i)] = topDevelopersViewActives.at(i);

    // Config - Commit History
    QStringList commitHistoryViewNames = m_configGroup.readEntry("commitHistoryViewNames", KdeObservatoryPresets::preset(KdeObservatoryPresets::ProjectName));
    QList<bool> commitHistoryViewActives = m_configGroup.readEntry("commitHistoryViewActives", KdeObservatoryPresets::automaticallyInViews());

    m_commitHistoryViewProjects.clear();
    int commitHistoryViewsCount = commitHistoryViewNames.count();
    for (int i = 0; i < commitHistoryViewsCount; ++i)
        m_commitHistoryViewProjects[commitHistoryViewNames.at(i)] = commitHistoryViewActives.at(i);

    // Config - Krazy Report
    QStringList krazyReportViewNames = m_configGroup.readEntry("krazyReportViewNames", KdeObservatoryPresets::preset(KdeObservatoryPresets::ProjectName));
    QList<bool> krazyReportViewActives = m_configGroup.readEntry("krazyReportViewActives", KdeObservatoryPresets::automaticallyInViews());

    m_krazyReportViewProjects.clear();
    int krazyReportViewsCount = krazyReportViewNames.count();
    for (int i = 0; i < krazyReportViewsCount; ++i)
        m_krazyReportViewProjects[krazyReportViewNames.at(i)] = krazyReportViewActives.at(i);

    commitCollector->setExtent(m_commitExtent);
    saveConfig();
}

void KdeObservatory::saveConfig()
{
    // General properties
    m_configGroup.writeEntry("commitExtent", m_commitExtent);
    m_configGroup.writeEntry("synchronizationDelay", m_synchronizationDelay);
    m_configGroup.writeEntry("enableAutoViewChange", m_enableAutoViewChange);
    m_configGroup.writeEntry("viewsDelay", m_viewsDelay);

    QStringList viewNames;
    QList<bool> viewActives;

    int count = m_activeViews.count();
    for (int i = 0; i < count; ++i)
    {
        const QPair<QString, bool> &pair = m_activeViews.at(i);
        viewNames << pair.first;
        viewActives << pair.second;
    }
    m_configGroup.writeEntry("viewNames", viewNames);
    m_configGroup.writeEntry("viewActives", viewActives);

    // Projects properties
    QStringList projectNames;
    QStringList projectCommitSubjects;
    QStringList projectKrazyReports;
    QStringList projectKrazyFilePrefix;
    QStringList projectIcons;

    QMapIterator<QString, Project> i(m_projects);
    while (i.hasNext())
    {
        i.next();
        const Project &project = i.value();
        projectNames << i.key();
        projectCommitSubjects << project.commitSubject;
        projectKrazyReports << project.krazyReport;
        projectKrazyFilePrefix << project.krazyFilePrefix;
        projectIcons << project.icon;
    }

    m_configGroup.writeEntry("projectNames", projectNames);
    m_configGroup.writeEntry("projectCommitSubjects", projectCommitSubjects);
    m_configGroup.writeEntry("projectKrazyReports", projectKrazyReports);
    m_configGroup.writeEntry("projectKrazyFilePrefix", projectKrazyFilePrefix);
    m_configGroup.writeEntry("projectIcons", projectIcons);

    m_configGroup.writeEntry("topActiveProjectsViewNames", m_topActiveProjectsViewProjects.keys());
    m_configGroup.writeEntry("topActiveProjectsViewActives", m_topActiveProjectsViewProjects.values());

    m_configGroup.writeEntry("topDevelopersViewNames", m_topDevelopersViewProjects.keys());
    m_configGroup.writeEntry("topDevelopersViewActives", m_topDevelopersViewProjects.values());

    m_configGroup.writeEntry("commitHistoryViewNames", m_commitHistoryViewProjects.keys());
    m_configGroup.writeEntry("commitHistoryViewActives", m_commitHistoryViewProjects.values());

    m_configGroup.writeEntry("krazyReportViewNames", m_krazyReportViewProjects.keys());
    m_configGroup.writeEntry("krazyReportViewActives", m_krazyReportViewProjects.values());

    emit configNeedsSaving();
}

void KdeObservatory::createTimers()
{
    m_viewTransitionTimer = new QTimer(this);
    m_viewTransitionTimer->setInterval(m_viewsDelay * 1000);
    connect(m_viewTransitionTimer, SIGNAL(timeout()), this, SLOT(moveViewRight()));

    m_synchronizationTimer = new QTimer(this);
    m_synchronizationTimer->setInterval(m_synchronizationDelay * 1000);
    connect(m_synchronizationTimer, SIGNAL(timeout()), this, SLOT(runCollectors()));
}

void KdeObservatory::createViewProviders()
{
    m_viewProviders[i18n("Top Active Projects")] = new TopActiveProjectsView(m_topActiveProjectsViewProjects, m_projects, m_viewContainer);
    m_viewProviders[i18n("Top Developers")] = new TopDevelopersView(m_topDevelopersViewProjects, m_projects, m_viewContainer);
    m_viewProviders[i18n("Commit History")] = new CommitHistoryView(m_commitHistoryViewProjects, m_projects, m_viewContainer);
    m_viewProviders[i18n("Krazy Report")] = new KrazyReportView(m_krazyReportViewProjects, m_projects, m_viewContainer);
}

#include "kdeobservatory.moc"
