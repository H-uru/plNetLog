/******************************************************************************
 * This file is part of plNetLog.                                             *
 *                                                                            *
 * plNetLog is free software: you can redistribute it and/or modify           *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * plNetLog is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with plNetLog.  If not, see <http://www.gnu.org/licenses/>.          *
 ******************************************************************************/

#include "ChunkBuffer.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <QMessageBox>

void ChunkBuffer::chomp(void* out, size_t size)
{
    m_mutex.lock();
    while (size) {
        waitOnData();

        Buffer& current = m_chunks.front();
        if (size >= (current.m_size - current.m_cur)) {
            size -= current.m_size - current.m_cur;
            memcpy(out, current.m_data + current.m_cur, current.m_size - current.m_cur);
            delete[] current.m_data;
            m_chunks.pop_front();
        } else {
            memcpy(out, current.m_data + current.m_cur, size);
            current.m_cur += size;
            size = 0;
        }
    }
    m_mutex.unlock();
}

void ChunkBuffer::skip(size_t size)
{
    m_mutex.lock();
    while (size) {
        waitOnData();

        Buffer& current = m_chunks.front();
        if (size >= (current.m_size - current.m_cur)) {
            size -= current.m_size - current.m_cur;
            m_chunks.pop_front();
        } else {
            current.m_cur += size;
            size = 0;
        }
    }
    m_mutex.unlock();
}

void ChunkBuffer::append(const unsigned char* data, size_t size, unsigned time)
{
    m_mutex.lock();
    m_chunks.push_back(Buffer());
    Buffer& buf = m_chunks.back();
    buf.m_cur = 0;
    buf.m_data = (const unsigned char*)data;
    buf.m_size = size;
    buf.m_time = time;
    m_mutex.unlock();
}

unsigned ChunkBuffer::currentTime()
{
    m_mutex.lock();
    waitOnData();
    unsigned time = m_chunks.front().m_time;
    m_mutex.unlock();
    return time;
}

QString ChunkBuffer::readResultCode()
{
    static QString errMap[] = {
        "kNetSuccess",                  "kNetErrInternalError",
        "kNetErrTimeout",               "kNetErrBadServerData",
        "kNetErrAgeNotFound",           "kNetErrConnectFailed",
        "kNetErrDisconnected",          "kNetErrFileNotFound",
        "kNetErrOldBuildId",            "kNetErrRemoteShutdown",
        "kNetErrTimeoutOdbc",           "kNetErrAccountAlreadyExists",
        "kNetErrPlayerAlreadyExists",   "kNetErrAccountNotFound",
        "kNetErrPlayerNotFound",        "kNetErrInvalidParameter",
        "kNetErrNameLookupFailed",      "kNetErrLoggedInElsewhere",
        "kNetErrVaultNodeNotFound",     "kNetErrMaxPlayersOnAcct",
        "kNetErrAuthenticationFailed",  "kNetErrStateObjectNotFound",
        "kNetErrLoginDenied",           "kNetErrCircularReference",
        "kNetErrAccountNotActivated",   "kNetErrKeyAlreadyUsed",
        "kNetErrKeyNotFound",           "kNetErrActivationCodeNotFound",
        "kNetErrPlayerNameInvalid",     "kNetErrNotSupported",
        "kNetErrServiceForbidden",      "kNetErrAuthTokenTooOld",
        "kNetErrMustUseGameTapClient",  "kNetErrTooManyFailedLogins",
        "kNetErrGameTapConnectionFailed", "kNetErrGTTooManyAuthOptions",
        "kNetErrGTMissingParameter",    "kNetErrGTServerError",
        "kNetErrAccountBanned",         "kNetErrKickedByCCR",
        "kNetErrScoreWrongType",        "kNetErrScoreNotEnoughPoints",
        "kNetErrScoreAlreadyExists",    "kNetErrScoreNoDataFound",
        "kNetErrInviteNoMatchingPlayer", "kNetErrInviteTooManyHoods",
        "kNetErrNeedToPay",             "kNetErrServerBusy",
        "kNetErrVaultNodeAccessViolation",
    };

    unsigned result = read<unsigned>();

    if (result < (sizeof(errMap) / sizeof(errMap[0])))
        return errMap[result];
    return QString("(INVALID RESULT CODE - %1)").arg(result);
}

void ChunkBuffer::waitOnData()
{
    while (m_chunks.size() == 0) {
        // Give the buffer some time to fill
        m_mutex.unlock();
        Sleep(50);
        m_mutex.lock();
    }
}