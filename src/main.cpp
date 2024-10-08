#include <kaboutdata.h>
#include <klocalizedcontext.h>
#include <qlockfile.h>
#include <qobject.h>
#include <qqml.h>
#include <qqmlapplicationengine.h>
#include <qquickstyle.h>
#include <qstandardpaths.h>
#include <qstringliteral.h>
#include <qtenvironmentvariables.h>
#include <qurl.h>
#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>
#include <QtQml>
#include <memory>
#include "config.h"
#include "eyeofsauron_db.h"
#include "sound_wave.hpp"
#include "tracker.hpp"
#include "util.hpp"

auto get_lock_file() -> std::unique_ptr<QLockFile> {
  auto lockFile = std::make_unique<QLockFile>(QString::fromStdString(
      QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString() + "/eyeofsauron.lock"));

  lockFile->setStaleLockTime(0);

  bool status = lockFile->tryLock(100);

  if (!status) {
    util::critical("Could not lock the file: " + lockFile->fileName().toStdString());

    switch (lockFile->error()) {
      case QLockFile::NoError:
        break;
      case QLockFile::LockFailedError: {
        util::critical("Another instance already has the lock");
        break;
      }
      case QLockFile::PermissionError: {
        util::critical("No permission to reate the lock file");
        break;
      }
      case QLockFile::UnknownError: {
        util::critical("Unknown error");
        break;
      }
    }
  }

  return lockFile;
}

void construct_about_window() {
  KAboutData aboutData(QStringLiteral(COMPONENT_NAME), i18nc("@title", APPLICATION_NAME),
                       QStringLiteral(PROJECT_VERSION),
                       i18n("Using webcams and Middle-earth's power in your Physics classes"), KAboutLicense::GPL_V3,
                       i18n("(c) 2024"), QStringLiteral(""), QStringLiteral("https://github.com/wwmm/eyeofsauron"),
                       QStringLiteral("https://github.com/wwmm/eyeofsauron/issues"));

  aboutData.addAuthor(i18nc("@info:credit", "Wellington Wallace"), i18nc("@info:credit", "Developer"),
                      QStringLiteral("wellingtonwallace@gmail.com"));

  // Set aboutData as information about the app
  KAboutData::setApplicationData(aboutData);

  qmlRegisterSingletonType("AboutEoS",  // How the import statement should look like
                           VERSION_MAJOR, VERSION_MINOR, "AboutEoS", [](QQmlEngine* engine, QJSEngine*) -> QJSValue {
                             return engine->toScriptValue(KAboutData::applicationData());
                           });
}

int main(int argc, char* argv[]) {
  auto lockFile = get_lock_file();

  if (!lockFile->isLocked()) {
    return -1;
  }

  QApplication app(argc, argv);

  KLocalizedString::setApplicationDomain(APPLICATION_DOMAIN);
  QCoreApplication::setOrganizationName(QStringLiteral(ORGANIZATION_NAME));
  QCoreApplication::setOrganizationDomain(QStringLiteral(ORGANIZATION_DOMAIN));
  QCoreApplication::setApplicationName(QStringLiteral("eyeofsauron"));

  if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
  }

  construct_about_window();

  // Registering kcfg settings

  auto db = db::Main::self();

  qmlRegisterSingletonInstance("EoSdb", VERSION_MAJOR, VERSION_MINOR, "EoSdb", db);

  // loading classes

  tracker::Backend tracker;
  sound::Backend sound;

  QQmlApplicationEngine engine;

  engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
  engine.load(QUrl(QStringLiteral("qrc:/ui/main.qml")));

  if (engine.rootObjects().isEmpty()) {
    return -1;
  }

  QObject::connect(&app, &QApplication::aboutToQuit, [=]() { db->save(); });

  return QApplication::exec();
}