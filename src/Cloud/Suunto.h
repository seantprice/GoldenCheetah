/*
 * Copyright (c) 2020 Sean Price.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef GC_Suunto_h
#define GC_Suunto_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

/*
* Suunto implementatation for CloudService.
*/
class Suunto : public CloudService {

	Q_OBJECT

	public:
		QString id() const { return "Suunto"; }
		QString uiName() const { return tr("Suunto"); }
		QString description() const { return "Sync activities and data from Suunto";  }
		QImage logo() const;

		Suunto(Context* context);
		CloudService* clone(Context* context) { return new Suunto(context); }
		~Suunto();

		// open/connect and close/disconnect
		bool open(QStringList& errors);
		bool close();

		// Suunto does not have an API for uploading. There is an API for measurements,
		// but it only supports steps and total energy consumption, so this service
		// doesn't use it.
		virtual int capabilities() const { return OAuth | Download | Query; }

		// TODO: replace this with official Suunto logo (or remove comment if approved).
		QString authiconpath() const { return QString(":images/services/suunto-logo.png"); }

		// ReadDir: get list of activities to download
		QList<CloudServiceEntry*> readdir(QString path, QStringList& errors, QDateTime from, QDateTime to);
		// ReadFile: download activity
		bool readFile(QByteArray* data, QString remotename, QString remoteid);

	public slots:

		// getting data
		void readyRead(); // a readFile operation has work to do
		void readFileCompleted();

	private:
		Context* context;
		QNetworkAccessManager* nam;
		QNetworkReply* reply;
		CloudServiceEntry* root_;
		QMap<QNetworkReply*, QByteArray*> buffers;

	private slots:
		void onSslErrors(QNetworkReply* reply, const QList<QSslError>& error);
};
#endif