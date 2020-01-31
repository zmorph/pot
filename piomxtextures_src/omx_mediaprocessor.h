/*
 * Project: PiOmxTextures
 * Author:  Luca Carlon
 * Date:    11.01.2012
 *
 * Copyright (c) 2012, 2013 Luca Carlon. All rights reserved.
 *
 * This file is part of PiOmxTextures.
 *
 * PiOmxTextures is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PiOmxTextures is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PiOmxTextures.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OMX_MEDIAPROCESSOR_H
#define OMX_MEDIAPROCESSOR_H

/*------------------------------------------------------------------------------
|    includes
+-----------------------------------------------------------------------------*/
#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVariantMap>
#include <QThreadPool>
#include <QMediaTimeRange>

#include <GLES2/gl2.h>
#include <stdexcept>
#include <memory>

#include "omx_logging.h"
#include "omx_qthread.h"
#include "omx_textureprovider.h"

using namespace std;

/*------------------------------------------------------------------------------
|    defintions
+-----------------------------------------------------------------------------*/
class OMX_TextureData;
class OMX_TextureProvider;
class OMXCore;
class OMXClock;
class OMXPlayerVideo;
class OMX_PlayerAudio;
#ifdef ENABLE_SUBTITLES
class OMXPlayerSubtitles;
#endif
class OMX_Reader;
class OMXPacket;
class AVFormatContext;
class AVStream;
class AVPacket;
class CRBP;
class COMXCore;
class COMXStreamInfo;
class OMXVideoConfig;
class OMXAudioConfig;

/*------------------------------------------------------------------------------
|    OMX_MediaProcessorHelper class
+-----------------------------------------------------------------------------*/
class OMX_MediaProcessorHelper : public QObject
{
	Q_OBJECT
public:
	OMX_MediaProcessorHelper(OMX_EGLBufferProviderSh provider, QThread* t) {
		moveToThread(t);
		m_provider = provider;
	}

	virtual ~OMX_MediaProcessorHelper() {}

public slots:
	void onFreeRequest();

private:
	OMX_EGLBufferProviderSh m_provider;
};

/*------------------------------------------------------------------------------
|    OMX_MediaProcessor class
+-----------------------------------------------------------------------------*/
/**
 * @brief The OMX_MediaProcessor class Plays the media passed to the constructor
 * using the HDMI as audio output and a texture as the rendering surface. The
 * ID of the texture is sent when it is ready.
 */
class OMX_MediaProcessor : public QObject
{
    Q_OBJECT
public:
    enum OMX_MediaProcessorState {
        STATE_STOPPED,
        STATE_INACTIVE,
        STATE_PAUSED,
        STATE_PLAYING
    };

	 enum OMX_MediaStatus {
		 MEDIA_STATUS_UNKNOWN,
		 MEDIA_STATUS_NO_MEDIA,
		 MEDIA_STATUS_LOADING,
		 MEDIA_STATUS_LOADED,
		 MEDIA_STATUS_STALLED,
		 MEDIA_STATUS_BUFFERING,
		 MEDIA_STATUS_BUFFERED,
		 MEDIA_STATUS_END_OF_MEDIA,
		 MEDIA_STATUS_INVALID_MEDIA
	 };

	 static const char* STATE_STR[];
	 static const char* M_STATUS[];

    enum OMX_MediaProcessorError {
        ERROR_CANT_OPEN_FILE,
        ERROR_WRONG_THREAD
    };

    OMX_MediaProcessor(OMX_EGLBufferProviderSh provider);
    virtual ~OMX_MediaProcessor();

	 bool setFilename(const QString& filename);
    QString filename();
    QStringList streams();

    qint64 streamPosition();

    bool hasAudio();
    bool hasVideo();
	 bool isSeekable();

    qint64 streamLength();

#ifdef ENABLE_SUBTITLES
    inline bool hasSubtitle() {
        return m_has_subtitle;
    }
#endif

    OMX_MediaProcessorState state();
	 OMX_MediaStatus mediaStatus();

    void setVolume(long volume, bool linear);
    long volume(bool linear);

    void setMute(bool muted);
	 bool muted();

    QVariantMap getMetaData();

    OMX_EGLBufferProviderSh m_provider;

public slots:
    bool play();
    bool stop();
    bool pause();
    bool seek(qint64 position);

signals:
	 void streamLengthChanged(qint64 length);
    void metadataChanged(const QVariantMap metadata);
    void playbackStarted();
    void playbackCompleted();
    void errorOccurred(OMX_MediaProcessor::OMX_MediaProcessorError error);
    void stateChanged(OMX_MediaProcessor::OMX_MediaProcessorState state);
	 void mediaStatusChanged(OMX_MediaProcessor::OMX_MediaStatus status);
	 void bufferStatusChanged(int percentage);
	 void availablePlaybackRangesChanged(QMediaTimeRange ranges);

private slots:
    void init();
	 OMX_MediaStatus setFilenameInt(const QString& filename);
	 bool setFilenameWrapper(const QString& filename);
    bool playInt();
    bool stopInt();
    bool pauseInt();
    bool seekInt(qint64 position);
    void mediaDecoding();
    void closeAll();
    bool cleanup();

private:
    void setState(OMX_MediaProcessorState state);
	 void setMediaStatus(OMX_MediaStatus status);
    void setSpeed(int iSpeed);
	 void flushStreams(double pts);
    void convertMetaData();

    OMX_QThread* m_thread;
    QString m_sourceUrl;

    AVFormatContext* fmt_ctx;
    AVStream* streamVideo;
    AVPacket* pkt;

    volatile OMX_MediaProcessorState m_state;
	 volatile OMX_MediaStatus m_mediaStatus;

    QMutex m_sendCmd;

	 OMXClock*           m_av_clock;
	 OMXPlayerVideo*     m_player_video;
	 OMX_PlayerAudio*    m_player_audio;
#ifdef ENABLE_SUBTITLES
    OMXPlayerSubtitles* m_player_subtitles;
#endif
	 OMX_Reader*         m_omx_reader;
    OMXPacket*          m_omx_pkt;

    CRBP*     m_RBP;
    COMXCore* m_OMX;

    bool m_has_video;
    bool m_has_audio;
#ifdef ENABLE_SUBTITLES
    bool m_has_subtitle;
#endif
    bool m_buffer_empty;
    bool m_pendingStop;
    bool m_pendingPause;
    bool m_pendingSeek;

    int m_subtitle_index;
    int m_audio_index;
	 int m_streamLength;

    QMutex m_mutexPending;
    QWaitCondition m_waitPendingCommand;

    volatile long m_incrMs;

    OMXAudioConfig* m_audioConfig;
    OMXVideoConfig* m_videoConfig;

    int m_playspeedCurrent;
    bool m_seekFlush;
    bool m_packetAfterSeek;
    double startpts;

    QVariantMap m_metadata;

	 bool m_muted;
	 double m_volume;
    float m_fps;

    // QtConcurrent uses a thread pool which on Pi1 counts only
    // 1 thread. I need a separate pool here.
    QThreadPool m_tpool;
};

/*------------------------------------------------------------------------------
|    OMX_MediaProcessor::hasAudio
+-----------------------------------------------------------------------------*/
inline bool OMX_MediaProcessor::hasAudio() {
    return m_has_audio;
}

/*------------------------------------------------------------------------------
|    OMX_MediaProcessor::hasVideo
+-----------------------------------------------------------------------------*/
inline bool OMX_MediaProcessor::hasVideo() {
    return m_has_video;
}

/*------------------------------------------------------------------------------
|    OMX_MediaProcessor::state
+-----------------------------------------------------------------------------*/
inline OMX_MediaProcessor::OMX_MediaProcessorState OMX_MediaProcessor::state() {
    return m_state;
}

/*------------------------------------------------------------------------------
|    OMX_MediaProcessor::mediaStatus
+-----------------------------------------------------------------------------*/
inline OMX_MediaProcessor::OMX_MediaStatus OMX_MediaProcessor::mediaStatus() {
	return m_mediaStatus;
}

/*------------------------------------------------------------------------------
|    OMX_MediaProcessor::setState
+-----------------------------------------------------------------------------*/
inline void OMX_MediaProcessor::setState(OMX_MediaProcessorState state) {
	if (m_state == state)
		return;

	log_verbose("State changing from %s to %s...", STATE_STR[m_state], STATE_STR[state]);
   m_state = state;
   emit stateChanged(state);
}

/*------------------------------------------------------------------------------
|    OMX_MediaProcessor::setMediaStatus
+-----------------------------------------------------------------------------*/
inline void OMX_MediaProcessor::setMediaStatus(OMX_MediaStatus status) {
	if (m_mediaStatus == status)
		 return;

	log_verbose("Media status changing from %s to %s...", M_STATUS[m_mediaStatus], M_STATUS[status]);
	m_mediaStatus = status;
	emit mediaStatusChanged(status);
}

/*------------------------------------------------------------------------------
|    OMX_MediaProcessor::getMetaData
+-----------------------------------------------------------------------------*/
inline QVariantMap OMX_MediaProcessor::getMetaData() {
   return m_metadata;
}

#endif // OMX_MEDIAPROCESSOR_H
