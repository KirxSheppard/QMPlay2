/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FFCommon.hpp>

#include <QProcess>
#include <QSysInfo>
#include <QDebug>
#include <QDir>

extern "C"
{
    #include <libavcodec/avcodec.h>
#ifdef QMPlay2_VDPAU
    #include <libavcodec/vdpau.h>
#endif
}

#ifdef QMPlay2_VDPAU
AVVDPAUContext *FFCommon::allocAVVDPAUContext(AVCodecContext *codecCtx)
{
    // Since FFmpeg 3.3 we must not use "av_vdpau_alloc_context()" or "AVVDPAUContext" structure size
    // for allocating "AVCodecContext::hwaccel_context", because internally it always uses field from
    // different internal structure which is larger. Using different struct inside FFmpeg was provided
    // earlier, but at least it was binary compatible with "AVVDPAUContext".
    // Since https://github.com/FFmpeg/FFmpeg/commit/7e4ba776a2240d40124d5540ea6b2118fa2fe26a it is no
    // longer binary compatible because of initialization of "reset" field of internal structure which
    // leads to buffer overflow. It causes random and weird crashes.
    if (av_vdpau_bind_context(codecCtx, 0, nullptr, 0) == 0)
        return (AVVDPAUContext *)codecCtx->hwaccel_context;
    return nullptr;
}
#endif

#ifdef FIND_HWACCEL_DRIVERS_PATH
void FFCommon::setDriversPath(const QString &dirName, const QByteArray &envVar)
{
    if (qEnvironmentVariableIsSet(envVar))
        return;

    static const auto libDirs = [] {
        QList<QByteArray> dirsWithWordSize, dirs;

        QProcess whereis;
        whereis.start("whereis", {"-l"});
        if (whereis.waitForStarted() && whereis.waitForFinished() && whereis.exitCode() == 0)
        {
            const auto wordSizeStr = QByteArray::number(QSysInfo::WordSize);
            for (const auto &entry : whereis.readAllStandardOutput().split('\n'))
            {
                if (!entry.startsWith("bin: "))
                    continue;

                const auto path = entry.mid(5);
                if (path.contains("/lib" + wordSizeStr))
                    dirsWithWordSize.append(path);
                else if (path.contains("/lib/") || path.endsWith("/lib"))
                    dirs.append(path);
            }
        }

        return dirsWithWordSize + dirs;
    }();

    for (auto &&dir : libDirs)
    {
        const QString path = dir + "/" + dirName;
        if (QDir(path).entryList({"*.so*"}, QDir::Files).count() > 0)
        {
            qputenv(envVar, path.toLocal8Bit());
            qDebug().noquote() << "Set" << envVar << "to" << path;
            break;
        }
    }
}
#endif
