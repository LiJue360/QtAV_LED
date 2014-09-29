/******************************************************************************
    QtAV:  Media play library based on Qt and LibASS
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include <stdarg.h>
#include <string>
extern "C" {
#include <ass/ass.h>
}
#include <QtDebug>
#include <QtGui/QPainter>
#include "QtAV/private/SubtitleProcessor.h"
#include "QtAV/prepost.h"
#include "QtAV/Packet.h"
#include "PlainText.h"

namespace QtAV {

class SubtitleProcessorLibASS : public SubtitleProcessor
{
public:
    SubtitleProcessorLibASS();
    virtual ~SubtitleProcessorLibASS();
    virtual SubtitleProcessorId id() const;
    virtual QString name() const;
    virtual QStringList supportedTypes() const;
    virtual bool process(QIODevice* dev);
    // supportsFromFile must be true
    virtual bool process(const QString& path);
    virtual QList<SubtitleFrame> frames() const;
    virtual QString getText(qreal pts) const;
    virtual QImage getImage(qreal pts, int width, int height);
    virtual bool processHeader(const QByteArray& data);
    virtual SubtitleFrame processLine(const QByteArray& data, qreal pts = -1, qreal duration = 0);
private:
    void processTrack(ASS_Track *track);
    ASS_Library *m_ass;
    ASS_Renderer *m_renderer;
    ASS_Track *m_track;
    // video frame width, height
    int m_width, m_height;
    QList<SubtitleFrame> m_frames;
};

static const SubtitleProcessorId SubtitleProcessorId_LibASS = "qtav.subtitle.processor.libass";
namespace {
static const std::string kName("LibASS");
}
FACTORY_REGISTER_ID_AUTO(SubtitleProcessor, LibASS, kName)

void RegisterSubtitleProcessorLibASS_Man()
{
    FACTORY_REGISTER_ID_MAN(SubtitleProcessor, LibASS, kName)
}

static void msg_callback(int level, const char *fmt, va_list va, void *data)
{
    Q_UNUSED(data)
    if (level > 6)
        return;
    printf("libass: ");
    vprintf(fmt, va);
    printf("\n");
}

SubtitleProcessorLibASS::SubtitleProcessorLibASS()
    : m_ass(0)
    , m_renderer(0)
    , m_track(0)
    , m_width(0)
    , m_height(0)
{
    m_ass = ass_library_init();
    if (!m_ass) {
        qWarning("ass_library_init failed!");
        return;
    }
    ass_set_message_cb(m_ass, msg_callback, NULL);
    m_renderer = ass_renderer_init(m_ass);
    if (!m_renderer) {
        qWarning("ass_renderer_init failed!");
        return;
    }

    //ass_set_frame_size(m_renderer, frame_w, frame_h);
    //ass_set_fonts(m_renderer, NULL, "Sans", 1, NULL, 1);
}

SubtitleProcessorLibASS::~SubtitleProcessorLibASS()
{
    if (m_track) {
        ass_free_track(m_track);
        m_track = 0;
    }
    if (m_renderer) {
        ass_renderer_done(m_renderer);
        m_renderer = 0;
    }
    if (m_ass) {
        ass_library_done(m_ass);
        m_ass = 0;
    }
}

SubtitleProcessorId SubtitleProcessorLibASS::id() const
{
    return SubtitleProcessorId_LibASS;
}

QString SubtitleProcessorLibASS::name() const
{
    return QString(kName.c_str());//SubtitleProcessorFactory::name(id());
}

QStringList SubtitleProcessorLibASS::supportedTypes() const
{
    // from LibASS/tests/fate/subtitles.mak
    // TODO: mp4
    static const QStringList sSuffixes = QStringList() << "ass" << "ssa";
    return sSuffixes;
}

QList<SubtitleFrame> SubtitleProcessorLibASS::frames() const
{
    return m_frames;
}

bool SubtitleProcessorLibASS::process(QIODevice *dev)
{
    if (m_track) {
        ass_free_track(m_track);
        m_track = 0;
    }
    if (!dev->isOpen()) {
        if (!dev->open(QIODevice::ReadOnly)) {
            qWarning() << "open qiodevice error: " << dev->errorString();
            return false;
        }
    }
    QByteArray data(dev->readAll());
    m_track = ass_read_memory(m_ass, (char*)data.constData(), data.size(), NULL); //utf-8
    if (!m_track) {
        qWarning("ass_read_memory error, ass track init failed!");
        return false;
    }
    processTrack(m_track);
    return true;
}


bool SubtitleProcessorLibASS::process(const QString &path)
{
    if (m_track) {
        ass_free_track(m_track);
        m_track = 0;
    }
    m_track = ass_read_file(m_ass, (char*)path.toUtf8().constData(), NULL);
    if (!m_track) {
        qWarning("ass_read_memory error, ass track init failed!");
        return false;
    }
    processTrack(m_track);
    return true;
}

bool SubtitleProcessorLibASS::processHeader(const QByteArray &data)
{
    // new track, ass_process_codec_private
    return false;
}

SubtitleFrame SubtitleProcessorLibASS::processLine(const QByteArray &data, qreal pts, qreal duration)
{
    //ass_process_data(track,...)
    SubtitleFrame frame;
    frame.begin = pts;
    frame.end = frame.begin + duration;
    return SubtitleFrame();
}

QString SubtitleProcessorLibASS::getText(qreal pts) const
{
    QString text;
    for (int i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].begin <= pts && m_frames[i].end >= pts) {
            text += m_frames[i].text + "\n";
            continue;
        }
        if (!text.isEmpty())
            break;
    }
    return text.trimmed();
}

QImage SubtitleProcessorLibASS::getImage(qreal pts, int width, int height)
{
    if (!m_ass) {
        qWarning("ass library not available");
        return QImage();
    }
    if (!m_renderer) {
        qWarning("ass renderer not available");
        return QImage();
    }
    if (!m_track) {
        qWarning("ass track not available");
        return QImage();
    }
    if (width != m_width || height != m_height) {
        m_width = width;
        m_height = height;
        ass_set_frame_size(m_renderer, width, height);
    }
    int detect_change = 0;
    ASS_Image *img = ass_render_frame(m_renderer, m_track, pts * 1000.0, &detect_change);
    QRect r(0, 0, 0, 0);
    ASS_Image *i = img;
    while (i) {
       r |= QRect(i->dst_x, i->dst_y, i->w, i->h);
       i = i->next;
    }
    QImage image(r.size(),QImage::Format_ARGB32);
    QPainter p(&image);
    i = img;
    while (i) {
        if (i->w > 0 && i->h > 0) { //w,h==0 means no paint
            // FIXME: last line maybe not padded to stride, so can not use qpainter?
            // rrggbbaa always
            p.drawImage(i->dst_x - r.x(), i->dst_y - r.y(), QImage(i->bitmap, i->w, i->h, i->stride, QImage::Format_RGBA8888));
        }
        i = i->next;
    }
    return image;
}

void SubtitleProcessorLibASS::processTrack(ASS_Track *track)
{
    // language, track type
    m_frames.clear();
    for (int i = 0; i < track->n_events; ++i) {
        SubtitleFrame frame;
        const ASS_Event& ae = track->events[i];
        frame.text = PlainText::fromAss(ae.Text);
        frame.begin = qreal(ae.Start)/1000.0;
        frame.end = frame.begin + qreal(ae.Duration)/1000.0;
        m_frames.append(frame);
    }
}

} //namespace QtAV