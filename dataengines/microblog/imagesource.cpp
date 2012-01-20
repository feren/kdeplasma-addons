/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "imagesource.h"


#include <KIO/Job>
#include <KImageCache>

ImageSource::ImageSource(QObject* parent)
    : Plasma::DataContainer(parent),
      m_imageCache(0)
{
    setObjectName(QLatin1String("UserImages"));
}

ImageSource::~ImageSource()
{
}

void ImageSource::loadImage(const QString &who, const KUrl &url)
{
    if (!m_imageCache) {
        m_imageCache = new KImageCache("plasma_engine_preview", 10485760); // Re-use previewengine's cache
    }
    //FIXME: since kio_http bombs the system with too many request put a temporary arbitrary limit here
    // revert as soon as BUG 192625 is fixed
    if (m_loadedPersons.contains(who)) {
        return;
    }
    const QString cacheKey = who + "@" + url.pathOrUrl();
    kDebug() << " LOAD IMAGE: " << who << url << cacheKey;
    // Check if the image is in the cache, if so return it
    QImage preview = QImage(QSize(48, 48), QImage::Format_ARGB32_Premultiplied);
    if (m_imageCache->findImage(cacheKey, &preview)) {
        // cache hit
        kDebug() << "Cache hit: " << cacheKey;
        setData(who, preview);
        emit dataChanged();
        checkForUpdate();
        return;
    }
    kDebug() << "Cache miss: " << cacheKey;

    m_loadedPersons << who;
    if (m_runningJobs < 500) {
        //if (who == "sebas") kDebug() << " 222 starting job" << who;
        m_runningJobs++;
        KIO::Job *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
        job->setAutoDelete(true);
        m_jobs[job] = who;
        connect(job, SIGNAL(data(KIO::Job*,QByteArray)),
                this, SLOT(recv(KIO::Job*,QByteArray)));
        connect(job, SIGNAL(result(KJob*)), this, SLOT(result(KJob*)));
    } else {
        m_queuedJobs.append(QPair<QString, KUrl>(who, url));
    }
}

void ImageSource::recv(KIO::Job* job, const QByteArray& data)
{
    m_jobData[job] += data;
}

void ImageSource::result(KJob *job)
{
    if (!m_jobs.contains(job)) {
        return;
    }

    m_runningJobs--;

    if (m_queuedJobs.count() > 0) {
        QPair<QString, KUrl> jobDesc = m_queuedJobs.takeLast();
        loadImage(jobDesc.first, jobDesc.second);
    }

    if (job->error()) {
        // TODO: error handling
    } else {
        QImage img;
        img.loadFromData(m_jobData.value(job));
        const QString who = m_jobs.value(job);
        setData(who, img);
        if (m_jobs.value(job) == "sebas") {
            kDebug() << " === SEBAS SET ===";
        }
        emit dataChanged();
        KIO::TransferJob* kiojob = dynamic_cast<KIO::TransferJob*>(job);
        const QString cacheKey = who + "@" + kiojob->url().pathOrUrl();
        kDebug() << "Cache insert: " << cacheKey;

        m_imageCache->insertImage(cacheKey, img);

    }

    m_jobs.remove(job);
    m_jobData.remove(job);
    checkForUpdate();
}


#include <imagesource.moc>

