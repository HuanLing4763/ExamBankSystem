#include "ExamBankSystem.h"
#include "env/ExamEnvManager.h"
#include "virtualbox/VirtualBoxController.h"
#include <QFileSystemWatcher>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QMouseEvent>
#include <QScreen>
#include <QPoint>
#include <QRect>
#include <QIcon>
#include <QWidget>
#include <QApplication>
#include <QStyle>

ExamBankSystem::ExamBankSystem(QWidget* parent) : QMainWindow(parent), isDraggingTitleBar(false)
{
	ui.setupUi(this);

	createTrayIcon();   // 创建托盘图标

	// 初始化自定义窗口控制按钮
	// 注意：图标路径硬编码，需确保资源文件.qrc中包含对应图片
	QIcon minimizeIcon(":/ExamBankSystem/resources/minimize.png");
	QIcon maximizeIcon(":/ExamBankSystem/resources/maximize.png");
	QIcon restoreIcon(":/ExamBankSystem/resources/restore.png");
	QIcon closeIcon(":/ExamBankSystem/resources/close.png");

	ui.minimizeButton->setIcon(minimizeIcon);
	ui.maximizeButton->setIcon(maximizeIcon);
	ui.closeButton->setIcon(closeIcon);

	// 为窗口控制按钮安装事件过滤器，用于自定义按钮行为
	ui.minimizeButton->installEventFilter(this);
	ui.maximizeButton->installEventFilter(this);
	ui.closeButton->installEventFilter(this);

	// 连接按钮点击事件与窗口操作
	connect(ui.minimizeButton, &QPushButton::clicked, this, &ExamBankSystem::showMinimized);
	connect(ui.maximizeButton, &QPushButton::clicked, this, [this, maximizeIcon, restoreIcon]() {
		if (isMaximized()) {
			showNormal();
			ui.maximizeButton->setIcon(maximizeIcon);
		}
		else {
			showMaximized();
			ui.maximizeButton->setIcon(restoreIcon);
		}
		});
	connect(ui.closeButton, &QPushButton::clicked, this, []() {
		QApplication::quit();
		});

	// 初始化功能页面
	specialPage = new PageSpecial(this);   // 专项练习页面
	examPage = new PageExam(this);         // 模拟考试页面
	examEnvPage = new PageExamEnv(this);   // 考试环境配置页面

	// 将页面添加到堆栈容器
	ui.mainContent->addWidget(specialPage);
	ui.mainContent->addWidget(examPage);
	ui.mainContent->addWidget(examEnvPage);
	ui.mainContent->setCurrentWidget(specialPage); // 默认显示专项练习

	// 连接侧边栏按钮事件
	connect(ui.btn_special, &QPushButton::clicked, this, &ExamBankSystem::onButtonClicked);
	connect(ui.btn_exam, &QPushButton::clicked, this, &ExamBankSystem::onButtonClicked);
	connect(ui.btn_manage, &QPushButton::clicked, this, &ExamBankSystem::onButtonClicked);
	connect(ui.label_special, &QPushButton::clicked, this, &ExamBankSystem::onButtonClicked);
	connect(ui.label_exam, &QPushButton::clicked, this, &ExamBankSystem::onButtonClicked);
	connect(ui.label_manage, &QPushButton::clicked, this, &ExamBankSystem::onButtonClicked);

	ui.sidebarHiddenPart->setMaximumWidth(0);  // 侧边栏初始隐藏

	// 为侧边栏按钮安装事件过滤器，用于实现按钮与标签的交互联动
	ui.btn_special->installEventFilter(this);
	ui.btn_exam->installEventFilter(this);
	ui.btn_manage->installEventFilter(this);
	ui.label_special->installEventFilter(this);
	ui.label_exam->installEventFilter(this);
	ui.label_manage->installEventFilter(this);

	// 配置侧边栏动画参数
	constexpr int ANIMATION_DURATION = 200; // 侧边栏动画标准时长

	expandAnimation = new QPropertyAnimation(ui.sidebarHiddenPart, "maximumWidth", this);
	expandAnimation->setDuration(ANIMATION_DURATION);     // 设置动画时长
	expandAnimation->setEasingCurve(QEasingCurve::InOutQuad);  // 平滑过渡曲线

	collapseAnimation = new QPropertyAnimation(ui.sidebarHiddenPart, "maximumWidth", this);
	collapseAnimation->setDuration(ANIMATION_DURATION);     // 设置动画时长
	collapseAnimation->setEasingCurve(QEasingCurve::InOutQuad);  // 平滑过渡曲线

	// 连接文件系统监视器，用于监视共享文件变化时的窗口更新
	QFileSystemWatcher* watcher = new QFileSystemWatcher(this);
	const QString sharedFolderPath = ExamEnvManager::getInstance().getSharedFolderPath();
	watcher->addPath(sharedFolderPath);
	
	connect(watcher, &QFileSystemWatcher::directoryChanged, [this, sharedFolderPath]() {
		QDir dir(sharedFolderPath);
		if (dir.exists("vm_shutdown_signal.flag")) {
			// 启动宿主机应用程序
			this->show();
			// 删除信号文件
			QFile::remove(dir.filePath("vm_shutdown_signal.flag"));
		}
		});

	// 监听虚拟机启动信号
	connect(VirtualBoxController::getInstance(), &VirtualBoxController::vmStarted, this, [this]() {
		qDebug() << "VM started";
		this->hide();
		}, Qt::QueuedConnection);
}


ExamBankSystem::~ExamBankSystem()
{}

/**
 * 页面路由逻辑：
 * - 按钮和标签均可触发切换
 * - 通过mainContent堆栈容器管理页面
 */
void ExamBankSystem::onButtonClicked() {
	QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
	if (!clickedButton) return;

	if (clickedButton == ui.btn_special || clickedButton == ui.label_special) {
		ui.mainContent->setCurrentWidget(specialPage);  // 切换到专项练习
	}
	else if (clickedButton == ui.btn_exam || clickedButton == ui.label_exam) {
		ui.mainContent->setCurrentWidget(examPage);     // 切换到模拟考试
	}
	else if (clickedButton == ui.btn_manage || clickedButton == ui.label_manage) {
		ui.mainContent->setCurrentWidget(examEnvPage);  // 切换到考试环境管理
	}
}

void ExamBankSystem::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		const QRect titleBarRect = ui.customTitleBar->rect();
		const QPoint titleBarTopLeft = ui.customTitleBar->mapToGlobal(titleBarRect.topLeft());
		const QPoint titleBarBottomRight = ui.customTitleBar->mapToGlobal(titleBarRect.bottomRight());
		const QRect globalTitleBarRect(titleBarTopLeft, titleBarBottomRight);

		// 检查鼠标点击是否在标题栏区域内，如果是，则设置窗口拖拽状态
		if (globalTitleBarRect.contains(event->globalPosition().toPoint())) {
			windowDragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
			isDraggingTitleBar = true; // 标记正在拖拽标题栏
			event->accept();
		}
	}
}

/**
 * @brief 处理窗口拖拽逻辑
 * @details 实现特性：
 * - 从最大化状态拖拽时智能复位窗口位置
 * - 动态计算窗口边界防止移出屏幕
 * - 处理多显示器环境坐标转换
 */
void ExamBankSystem::mouseMoveEvent(QMouseEvent* event)
{
	if ((event->buttons() & Qt::LeftButton) && isDraggingTitleBar) {
		const QScreen* currentScreen = window()->screen();
		const QRect screenGeometry = currentScreen->availableGeometry();
		QPoint mousePos = event->globalPosition().toPoint();

		/* 最大化窗口拖拽处理：
		 * 1. 根据标题栏点击位置比例计算复位窗口尺寸
		 * 2. 强制窗口宽度为屏幕50%或最小宽度（取较大值）
		 * 3. 固定窗口顶部对齐屏幕顶端
		 */
		if (windowState() & Qt::WindowMaximized) {
			// 计算鼠标在标题栏的横向位置比例（0.0~1.0）
			const QPointF localPos = ui.customTitleBar->mapFromGlobal(event->globalPosition());
			const double xRatio = localPos.x() / ui.customTitleBar->width();

			// 退出最大化状态
			showNormal();
			ui.maximizeButton->setIcon(QIcon(":/ExamBankSystem/resources/maximize.png"));

			// 窗口宽度计算
			const int newWidth = qMax(minimumWidth(), screenGeometry.width() / 2);

			// 根据横向比例计算窗口水平定位点
			const int newX = event->globalPosition().x() - (newWidth * xRatio);

			// 根据标题栏点击位置比例定位窗口
			move(qBound(screenGeometry.left(), newX,
				screenGeometry.right() - newWidth),
				screenGeometry.top()); // Y坐标固定屏幕顶端

			windowDragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
		}

		// 防止窗口被拖出屏幕底部
		if (mousePos.y() >= screenGeometry.bottom()) {
			QCursor::setPos(QPoint(mousePos.x(), screenGeometry.bottom()));
			move(mousePos.x() - windowDragPosition.x(), frameGeometry().top());
			return;
		}

		// 常规拖拽模式
		move(mousePos - windowDragPosition);
		event->accept();
	}
}

void ExamBankSystem::mouseReleaseEvent(QMouseEvent* event)
{
	isDraggingTitleBar = false;
	windowDragPosition = QPoint();
	event->accept();
}

/**
 * @brief 处理按钮与关联标签的交互联动事件
 * @details 当按钮或标签被悬停、按下或离开时，改变它们的背景颜色，并同步样式。
 */
bool ExamBankSystem::eventFilter(QObject* obj, QEvent* event)
{
	// 静态映射表，关联按钮与其对应的标签
	static std::unordered_map<QPushButton*, QPushButton*> buttonMap = {
		{ui.btn_special, ui.label_special},
		{ui.label_special, ui.btn_special},
		{ui.btn_exam, ui.label_exam},
		{ui.label_exam, ui.btn_exam},
		{ui.btn_manage, ui.label_manage},
		{ui.label_manage, ui.btn_manage}
	};

	QPushButton* currentButton = qobject_cast<QPushButton*>(obj);
	if (!currentButton) {
		return QWidget::eventFilter(obj, event);
	}

	QPushButton* associatedButton = buttonMap[currentButton];

	// 根据事件类型对按钮和其关联标签进行样式调整
	if (associatedButton) {
		if (event->type() == QEvent::Enter) {
			currentButton->setStyleSheet("background-color: #5dade2;");
			associatedButton->setStyleSheet("background-color: #5dade2;");
		}
		else if (event->type() == QEvent::Leave) {
			currentButton->setStyleSheet("background-color: transparent;");
			associatedButton->setStyleSheet("background-color: #3498db;");
		}
		else if (event->type() == QEvent::MouseButtonPress) {
			currentButton->setStyleSheet("background-color: #2980b9;");
			associatedButton->setStyleSheet("background-color: #2980b9;");
		}
		else if (event->type() == QEvent::MouseButtonRelease) {
			currentButton->setStyleSheet("background-color: transparent;");
			associatedButton->setStyleSheet("background-color: #3498db;");
		}
	}

	// 检查是否为侧边栏按钮的悬停或离开事件，如果是，则触发侧边栏的展开或收拢动画
	if ((obj == ui.btn_special || obj == ui.btn_exam || obj == ui.btn_manage ||
		obj == ui.label_special || obj == ui.label_exam || obj == ui.label_manage) &&
		(event->type() == QEvent::Enter || event->type() == QEvent::Leave)) {
		handleButtonEnterLeaveEvent(obj, event);
		return true;
	}

	return QWidget::eventFilter(obj, event);
}

void ExamBankSystem::handleButtonEnterLeaveEvent(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::Enter) {
		expandAnimation->setStartValue(ui.sidebarHiddenPart->maximumWidth());
		expandAnimation->setEndValue(120);
		expandAnimation->start();
	}
	else if (event->type() == QEvent::Leave) {
		collapseAnimation->setStartValue(ui.sidebarHiddenPart->maximumWidth());
		collapseAnimation->setEndValue(0);
		collapseAnimation->start();
	}
}

void ExamBankSystem::createTrayIcon() {
	trayIcon = new QSystemTrayIcon(this);

	trayIcon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

	QMenu* trayMenu = new QMenu(this);
	QAction* exitAction = new QAction("退出", this);
	connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
	trayMenu->addAction(exitAction);

	trayIcon->setContextMenu(trayMenu);
	trayIcon->show();
}

void ExamBankSystem::onExitApp() {
	QApplication::quit();
}