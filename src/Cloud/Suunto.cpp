
#include "Suunto.h"
#include "Settings.h"
#include "Secrets.h"
#include "Athlete.h"

#ifndef SUUNTO_DEBUG
#define SUUNTO_DEBUG true
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (SUUNTO_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (SUUNTO_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif


Suunto::Suunto(Context *context) : CloudService(context), context(context), root_(NULL) {
    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError>&)));
    }

    downloadCompression = none;
    filetype = uploadType::FIT;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_SUUNTO_TOKEN);
    settings.insert(Local1, GC_SUUNTO_REFRESH_TOKEN);
}

Suunto::~Suunto() {
    if (context) delete nam;
}

QImage Suunto::logo() const
{
    return QImage(":images/services/suunto-logo.png");
}

bool
Suunto::open(QStringList& errors)
{
    printd("Suunto::open\n");
    QString token = getSetting(GC_SUUNTO_REFRESH_TOKEN, "").toString();
    if (token == "") {
        errors << tr("No authorisation token configured.");
        return false;
    }

    printd("Get access token for this session.\n");

    // refresh endpoint
    QNetworkRequest request(QUrl("https://cloudapi-oauth.suunto.com/oauth/token?"));
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    // set params
    QString data;
    data += "&refresh_token=" + getSetting(GC_SUUNTO_REFRESH_TOKEN).toString();
    data += "&grant_type=refresh_token";

    QString authheader = QString("%1:%2").arg(GC_SUUNTO_CLIENT_ID).arg(GC_SUUNTO_CLIENT_SECRET);
    request.setRawHeader("Authorization", "Basic " + authheader.toLatin1().toBase64());

    // make request
    QNetworkReply* reply = nam->post(request, data.toLatin1());

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("HTTP response code: %d\n", statusCode);

    // oops, no dice
    if (reply->error() != 0) {
        printd("Got error %s\n", reply->errorString().toStdString().c_str());
        errors << reply->errorString();
        return false;
    }

    // lets extract the access token, and possibly a new refresh token
    QByteArray r = reply->readAll();
    printd("Got response: %s\n", r.data());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // failed to parse result !?
    if (parseError.error != QJsonParseError::NoError) {
        printd("Parse error!\n");
        errors << tr("JSON parser error") << parseError.errorString();
        return false;
    }

    QString access_token = document.object()["access_token"].toString();
    QString refresh_token = document.object()["refresh_token"].toString();

    // update our settings
    if (access_token != "") setSetting(GC_SUUNTO_TOKEN, access_token);
    if (refresh_token != "") setSetting(GC_SUUNTO_REFRESH_TOKEN, refresh_token);

    // get the factory to save our settings permanently
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

void
Suunto::onSslErrors(QNetworkReply* reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

bool
Suunto::close()
{
    printd("Suunto::close\n");
    // No action to take on close
    return true;
}

QList<CloudServiceEntry*>
Suunto::readdir(QString path, QStringList& errors, QDateTime from, QDateTime to)
{
    printd("Suunto::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_SUUNTO_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Suunto first");
        return returning;
    }

    // Do Paginated Access to the Activities List
    const int pageSize = 30;
    int offset = 0;
    int resultCount = INT_MAX;

    while (offset < resultCount) {
        QString urlstr = "https://cloudapi.suunto.com/v2/workouts?";

        QUrlQuery params;

        // Get since/until values (start/end of time range) from the to/from date selectors
        // Suunto uses milliseconds for the time, unlike Strava which uses seconds.
        params.addQueryItem("until", QString::number(to.addDays(1).toMSecsSinceEpoch(), 'f', 0));
        params.addQueryItem("since", QString::number(from.addDays(-1).toMSecsSinceEpoch(), 'f', 0));
        params.addQueryItem("limit", QString("%1").arg(pageSize));
        params.addQueryItem("offset", QString("%1").arg(offset));
        // set filter-by-modification-time to false, we care about when the workout was started, not when it was modified.
        params.addQueryItem("filter-by-modification-time", QString("false"));

        QUrl url = QUrl(urlstr + params.toString());
        printd("URL used: %s\n", url.url().toStdString().c_str());

        // request using the bearer token and the API subscription key
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        request.setRawHeader("Ocp-Apim-Subscription-Key", GC_SUUNTO_SUBSCRIPTION_KEY);

        QNetworkReply* reply = nam->get(request);

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "error" << reply->errorString();
            errors << tr("Network Problem reading Suunto data");
            //return returning;
        }
        // did we get a good response ?
        QByteArray r = reply->readAll();
        printd("response: %s\n", r.toStdString().c_str());

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        // if path was returned all is good, lets set root
        if (parseError.error == QJsonParseError::NoError) {

            // results ?
            QJsonValue payload = document["payload"];
            QJsonArray results = payload.toArray();

            // lets look at that then
            if (results.size() > 0) {
                for (int i = 0; i < results.size(); i++) {
                    QJsonObject each = results.at(i).toObject();
                    CloudServiceEntry* add = newCloudServiceEntry();

                    // Suunto doesn't use file path or names by default, just get activity ID.
                    add->label = QString(each["activityId"].toString());
                    add->id = QString(each["workoutKey"].toString());

                    add->isDir = false;
                    add->distance = each["totalDistance"].toDouble() / 1000.0;
                    // Cast duration to long (required by CloudServiceEntry), Suunto reports it as a double
                    add->duration = (long) each["totalTime"].toDouble();
                    add->name = QDateTime::fromMSecsSinceEpoch(each["startTime"].toVariant().toULongLong()).toString("yyyy_MM_dd_HH_mm_ss") + ".fit";

                    printd("direntry: %s %s\n", add->id.toStdString().c_str(), add->name.toStdString().c_str());

                    returning << add;
                }
                // next page
                offset += results.size();
            }
            else {
                offset = INT_MAX;
            }
        }
        else {
            // we had a parsing error - so something is wrong - stop requesting more data by ending the loop
            offset = INT_MAX;
        }
    }
    return returning;
}

bool
Suunto::readFile(QByteArray* data, QString remotename, QString remoteid) {
    printd("Suunto::readFile(%s)\n", remotename.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_SUUNTO_TOKEN, "").toString();
    if (token == "") return false;

    // Attempt to lookup workout via ID.
    QString url = QString("https://cloudapi.suunto.com/v2/workout/exportFit/%1").arg(remoteid);

    printd("url:%s\n", url.toStdString().c_str());

    // request using the bearer token and the API subscription key
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Ocp-Apim-Subscription-Key", GC_SUUNTO_SUBSCRIPTION_KEY);

    QNetworkReply* reply = nam->get(request);

    // Map reply for later processing.
    mapReply(reply, remotename);
    buffers.insert(reply, data);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    return true;
}

void
Suunto::readyRead()
{
    QNetworkReply* reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
Suunto::readFileCompleted()
{
    printd("Suunto::readFileCompleted\n");

    QNetworkReply* reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", buffers.value(reply)->toStdString().c_str());

    // No preprocessing required, just import the fit file directly.
    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}

static bool addSuunto() {
    CloudServiceFactory::instance().addService(new Suunto(NULL));
    return true;
}

static bool add = addSuunto();