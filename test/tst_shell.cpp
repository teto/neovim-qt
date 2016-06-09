#include <QtTest/QtTest>
#include <QLocalSocket>
#include <gui/mainwindow.h>
#include <msgpackrequest.h>
#include "common.h"

namespace NeovimQt {

class Test: public QObject
{
	Q_OBJECT
private slots:
	void benchStart() {
		QBENCHMARK {
			NeovimConnector *c = NeovimConnector::spawn();
			QSignalSpy onReady(c, SIGNAL(ready()));
			QVERIFY(onReady.isValid());
			QVERIFY(SPYWAIT(onReady));

			Shell *s = new Shell(c);
			QSignalSpy onResize(s, SIGNAL(neovimResized(int, int)));
			QVERIFY(onResize.isValid());
			QVERIFY(SPYWAIT(onResize));
		}
	}

	void uiStart() {
		QStringList args;
		args << "-u" << "NONE";
		NeovimConnector *c = NeovimConnector::spawn(args);
		Shell *s = new Shell(c);
		QSignalSpy onAttached(s, SIGNAL(neovimAttached(bool)));
		QVERIFY(onAttached.isValid());
		QVERIFY(SPYWAIT(onAttached));
		QVERIFY(s->neovimAttached());

		s->repaint();

		QPixmap p = s->grab();
		p.save("tst_shell_start.jpg");

	}

	void startVarsShellWidget() {
		QStringList args = {"-u", "NONE"};
		NeovimConnector *c = NeovimConnector::spawn(args);
		Shell *s = new Shell(c);
		QSignalSpy onAttached(s, SIGNAL(neovimAttached(bool)));
		QVERIFY(onAttached.isValid());
		QVERIFY(SPYWAIT(onAttached));
		QVERIFY(s->neovimAttached());
		checkStartVars(c);
	}

	void startVarsMainWindow() {
		QStringList args = {"-u", "NONE"};
		NeovimConnector *c = NeovimConnector::spawn(args);
		MainWindow *s = new MainWindow(c);
		QSignalSpy onAttached(s, SIGNAL(neovimAttached(bool)));
		QVERIFY(onAttached.isValid());
		QVERIFY(SPYWAIT(onAttached));
		QVERIFY(s->neovimAttached());
		checkStartVars(c);
	}

	void guiShimCommands() {
		QStringList args = {"-u", "NONE", "--cmd", "set rtp+=../src/gui/runtime"};
		NeovimConnector *c = NeovimConnector::spawn(args);
		MainWindow *s = new MainWindow(c);
		QSignalSpy onAttached(s, SIGNAL(neovimAttached(bool)));
		QVERIFY(onAttached.isValid());
		QVERIFY(SPYWAIT(onAttached));
		QVERIFY(s->neovimAttached());

		connect(c->neovimObject(), &Neovim::on_vim_get_option, [](QVariant rtp){
				qDebug() << rtp;
		});

		checkCommand(c, "GuiFont");
		c->neovimObject()->vim_get_option("runtimepath");
		checkCommand(c, "GuiLinespace");
	}

protected:

	void checkCommand(NeovimConnector *c, const QString& cmd) {
		auto req = c->neovimObject()->vim_command_output(c->encode(cmd));
		QSignalSpy cmdOk(req, SIGNAL(finished(quint32, Function::FunctionId, QVariant)));
		QVERIFY(cmdOk.isValid());
		QObject::connect(c->neovimObject(), &Neovim::err_vim_command_output, [cmd](QString msg, const QVariant& err) {
				qDebug() << cmd << msg;
			});
		qDebug() << cmd;
		// Force the Neovim event loop to advance with async vim_input()
		c->neovimObject()->vim_input("<cr>");
		QVERIFY(SPYWAIT(cmdOk));
		qDebug() << cmdOk << cmdOk.size();
		// Make sure the command output is not empty
		QVERIFY(!cmdOk.at(0).at(2).toByteArray().isEmpty());
	}

	/// Check for the presence of the GUI variables in Neovim
	void checkStartVars(NeovimQt::NeovimConnector *conn) {
		NeovimQt::Neovim *nvim = conn->neovimObject();
		connect(nvim, &NeovimQt::Neovim::err_vim_get_var,
			[](const QString& err, const QVariant& v) {
				qDebug() << err<< v;
			});

		QStringList vars = {"GuiWindowId", "GuiWindowMaximized",
			"GuiWindowFullScreen", "GuiFont"};
		foreach(const QString& var, vars) {
			qDebug() << "Checking Neovim for Gui var" << var;
			QSignalSpy onVar(nvim, &NeovimQt::Neovim::on_vim_get_var);
			QVERIFY(onVar.isValid());
			nvim->vim_get_var(conn->encode(var));
			QVERIFY(SPYWAIT(onVar));
		}
	}
};

} // Namespace NeovimQt
QTEST_MAIN(NeovimQt::Test)
#include "tst_shell.moc"
