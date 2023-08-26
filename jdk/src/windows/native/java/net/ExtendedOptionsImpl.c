/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <jni.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>

#include "net_util.h"
#include "jni_util.h"

static jfieldID sf_fd_fdID;             /* FileDescriptor.fd */

static void handleError(JNIEnv *env, jint rv, const char *errmsg) {
    if (rv < 0) {
        int error = WSAGetLastError();
        if (error == WSAENOPROTOOPT) {
            JNU_ThrowByName(env, "java/lang/UnsupportedOperationException",
                    "unsupported socket option");
        } else {
            JNU_ThrowByNameWithLastError(env, "java/net/SocketException", errmsg);
        }
    }
}

static jint socketOptionSupported(jint level, jint optname) {
    WSADATA wsaData;
    jint error = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (error != 0) {
        return 0;
    }

    SOCKET sock;
    jint one = 1;
    jint rv;
    socklen_t sz = sizeof(one);

    /* First try IPv6; fall back to IPv4. */
    sock = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        error = WSAGetLastError();
        if (error == WSAEPFNOSUPPORT || error == WSAEAFNOSUPPORT) {
            sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        }
        if (sock == INVALID_SOCKET) {
            return 0;
        }
    }

    rv = getsockopt(sock, level, optname, (char*) &one, &sz);
    error = WSAGetLastError();

    if (rv != 0 && error == WSAENOPROTOOPT) {
        rv = 0;
    } else {
        rv = 1;
    }

    closesocket(sock);
    WSACleanup();

    return rv;
}

/*
 * Class:     sun_net_ExtendedOptionsImpl
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_sun_net_ExtendedOptionsImpl_init
  (JNIEnv *env, jclass UNUSED) {

    static int initialized = 0;
    jclass c;

    /* Global class references */

    if (initialized) {
        return;
    }

    /* int "fd" field of java.io.FileDescriptor  */

    c = (*env)->FindClass(env, "java/io/FileDescriptor");
    CHECK_NULL(c);
    sf_fd_fdID = (*env)->GetFieldID(env, c, "fd", "I");
    CHECK_NULL(sf_fd_fdID);

    initialized = JNI_TRUE;
}

/*
 * Retrieve the int file-descriptor from a public socket type object.
 * Gets impl, then the FileDescriptor from the impl, and then the fd
 * from that.
 */
static int getFD(JNIEnv *env, jobject fileDesc) {
    return (*env)->GetIntField(env, fileDesc, sf_fd_fdID);
}

/* Non Solaris. Functionality is not supported. So, throw UnsupportedOpExc */

JNIEXPORT void JNICALL Java_sun_net_ExtendedOptionsImpl_setFlowOption
  (JNIEnv *env, jclass UNUSED, jobject fileDesc, jobject flow) {
    JNU_ThrowByName(env, "java/lang/UnsupportedOperationException",
        "unsupported socket option");
}

JNIEXPORT void JNICALL Java_sun_net_ExtendedOptionsImpl_getFlowOption
  (JNIEnv *env, jclass UNUSED, jobject fileDesc, jobject flow) {
    JNU_ThrowByName(env, "java/lang/UnsupportedOperationException",
        "unsupported socket option");
}

static jboolean flowSupported0()  {
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_sun_net_ExtendedOptionsImpl_flowSupported
  (JNIEnv *env, jclass UNUSED) {
    return JNI_FALSE;
}

/* Keepalive options not supported */

/*
 * Class:     sun_net_ExtendedOptionsImpl
 * Method:    keepAliveOptionsSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_sun_net_ExtendedOptionsImpl_keepAliveOptionsSupported
(JNIEnv *env, jobject unused) {
    return socketOptionSupported(IPPROTO_TCP, TCP_KEEPIDLE) && socketOptionSupported(IPPROTO_TCP, TCP_KEEPCNT)
            && socketOptionSupported(IPPROTO_TCP, TCP_KEEPINTVL);
}

/*
 * Class:     sun_net_ExtendedOptionsImpl
 * Method:    setTcpKeepAliveProbes
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_sun_net_ExtendedOptionsImpl_setTcpKeepAliveProbes
(JNIEnv *env, jobject unused, jobject fileDesc, jint optval) {
    int fd = getFD(env, fileDesc);
    jint rv = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (char*) &optval, sizeof(optval));
    handleError(env, rv, "set option TCP_KEEPCNT failed");
}

/*
 * Class:     sun_net_ExtendedOptionsImpl
 * Method:    setTcpKeepAliveTime
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_sun_net_ExtendedOptionsImpl_setTcpKeepAliveTime
(JNIEnv *env, jobject unused, jobject fileDesc, jint optval) {
    int fd = getFD(env, fileDesc);
    jint rv = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (char*) &optval, sizeof(optval));
    handleError(env, rv, "set option TCP_KEEPIDLE failed");
}

/*
 * Class:     sun_net_ExtendedOptionsImpl
 * Method:    setTcpKeepAliveIntvl
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_sun_net_ExtendedOptionsImpl_setTcpKeepAliveIntvl
(JNIEnv *env, jobject unused, jobject fileDesc, jint optval) {
    int fd = getFD(env, fileDesc);
    jint rv = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (char*) &optval, sizeof(optval));
    handleError(env, rv, "set option TCP_KEEPINTVL failed");
}

/*
 * Class:     sun_net_ExtendedOptionsImpl
 * Method:    getTcpKeepAliveProbes
 * Signature: (I)I;
 */
JNIEXPORT jint JNICALL Java_sun_net_ExtendedOptionsImpl_getTcpKeepAliveProbes
(JNIEnv *env, jobject unused, jobject fileDesc) {
    int fd = getFD(env, fileDesc);
    jint optval, rv;
    socklen_t sz = sizeof(optval);
    rv = getsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (char*) &optval, &sz);
    handleError(env, rv, "get option TCP_KEEPCNT failed");
    return optval;
}

/*
 * Class:     sun_net_ExtendedOptionsImpl
 * Method:    getTcpKeepAliveTime
 * Signature: (I)I;
 */
JNIEXPORT jint JNICALL Java_sun_net_ExtendedOptionsImpl_getTcpKeepAliveTime
(JNIEnv *env, jobject unused, jobject fileDesc) {
    int fd = getFD(env, fileDesc);
    jint optval, rv;
    socklen_t sz = sizeof(optval);
    rv = getsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (char*) &optval, &sz);
    handleError(env, rv, "get option TCP_KEEPIDLE failed");
    return optval;
}

/*
 * Class:     sun_net_ExtendedOptionsImpl
 * Method:    getTcpKeepAliveIntvl
 * Signature: (I)I;
 */
JNIEXPORT jint JNICALL Java_sun_net_ExtendedOptionsImpl_getTcpKeepAliveIntvl
(JNIEnv *env, jobject unused, jobject fileDesc) {
    int fd = getFD(env, fileDesc);
    jint optval, rv;
    socklen_t sz = sizeof(optval);
    rv = getsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (char*) &optval, &sz);
    handleError(env, rv, "get option TCP_KEEPINTVL failed");
    return optval;
}
