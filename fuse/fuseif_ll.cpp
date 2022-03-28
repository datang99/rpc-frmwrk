/*
 * =====================================================================================
 *
 *       Filename:  fuseif_ll.cpp
 *
 *    Description:  implementations of lower-level operations for read/write
 *
 *        Version:  1.0
 *        Created:  03/16/2022 10:17:06 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
/*
  part of the code is from FUSE project.
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Implementation of the high-level FUSE API on top of the low-level
  API.

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/
#include "fuseif.h"
extern "C" {

struct fuse_session_dup {
    char *mountpoint;
    volatile int exited;
    int fd;
    struct mount_opts *mo;
    int debug;
    int deny_others;
    struct fuse_lowlevel_ops op;
    int got_init;
    struct cuse_data *cuse_data;
    void *userdata;
    uid_t owner;
    struct fuse_conn_info conn;
    struct fuse_req list;
    struct fuse_req interrupts;
    pthread_mutex_t lock;
    int got_destroy;
    pthread_key_t pipe_key;
    int broken_splice_nonblock;
    uint64_t notify_ctr;
    struct fuse_notify_req notify_list;
    size_t bufsize;
    int error;
};

struct fuseif_intr_data {
    pthread_t id;
    pthread_cond_t cond;
    int finished;
    CFuseObjBase* fe;
};

}

void fuseif_free_buf( fuse_bufvec* buf )
{ free( buf ); }
static size_t fuseif_buf_size(const fuse_bufvec *bufv)
{
    size_t i;
    size_t size = 0;

    for (i = bufv->idx; i < bufv->count; i++) {
        if (bufv->buf[i].size == SIZE_MAX)
        {
            size = SIZE_MAX;
            break;
        }
        else
        {
            size += bufv->buf[i].size;
            if( i == bufv->idx )
                size -= bufv->off;
        }
    }

    return size;
}

static void fuseif_rwinterrupt(
    fuse_req_t req, void *d_)
{
    struct fuseif_intr_data *d = d_;
    struct fuse *f = req_fuse(req);

    if (d->id == pthread_self())
        return;

    CFuseObjBase* pObj = d->fe;
    while( pObj != nullptr )
    {
        CFuseStmFile* pstm = dynamic_cast
            < CFuseStmFile* >( pObj );
        if( pstm != nullptr )
        {
            CWriteLock oLock( pstm->GetLock() );
            pstm->CancelRequest( req );
            break;
        }
        CFuseFileEntry* pfe = dynamic_cast
            < CFuseFileEntry* >( pObj );
        {
            CWriteLock oLock( pfe->GetLock() );
            pfe->CancelRequest( req );
            break;
        }
        break;
    }
    pthread_mutex_lock(&f->lock);
    while (!d->finished) {
        struct timeval now;
        struct timespec timeout;

        pthread_kill(d->id, f->conf.intr_signal);
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;
        pthread_cond_timedwait(&d->cond, &f->lock, &timeout);
    }
    pthread_mutex_unlock(&f->lock);
}

static void fuseif_do_finish_interrupt(
    fuse *f, fuse_req_t req,
    fuseif_intr_data *d)
{
    pthread_mutex_lock(&f->lock);
    d->finished = 1;
    pthread_cond_broadcast(&d->cond);
    pthread_mutex_unlock(&f->lock);
    fuse_req_interrupt_func(req, NULL, NULL);
    pthread_cond_destroy(&d->cond);
}

static void fuseif_do_prepare_interrupt(
    fuse_req_t req,
    fuseif_intr_data *d)
{   
    d->id = pthread_self();
    pthread_cond_init(&d->cond, NULL);
    d->finished = 0;
    fuse_req_interrupt_func(req, fuseif_rwinterrupt, d);
}       
        
static inline void fuseif_finish_interrupt(
    fuse *f, fuse_req_t req,
    fuseif_intr_data *d)
{   
    if (f->conf.intr)
        fuseif_do_finish_interrupt(f, req, d);
}       

static inline void fuseif_prepare_interrupt(
    fuse *f, fuse_req_t req,
    fuseif_intr_data *d)
{              
    if (f->conf.intr)
        fuseif_do_prepare_interrupt(req, d);
}

void fuseif_ll_read(fuse_req_t req,
    fuse_ino_t ino, size_t size,
    off_t off, fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        fuse* pFuse = GetFuse();
        fuseif_intr_data* d = new fuseif_intr_data;
        d->fe = ( CFuseObjBase* )fi->fh;
        fuseif_prepare_interrupt( pFuse, req, d );
        fuse_bufvec* buf = nullptr;
        std::vector< BufPtr > vecBackup;
        ret = d->fe->fs_read(
            req, buf, off, size, fi, vecBackup, d );
        if( ret == STATUS_PENDING )
        {
            ret = 0;
            break;
        }
        fuseif_finish_interrupt( pFuse, req, d );
        delete d;
        if( SUCCEEDED( ret ) )
        {
            if( buf != nullptr )
                fuse_reply_data(req,
                    buf, FUSE_BUF_SPLICE_MOVE);
            else
                fuse_reply_err( req, 0 );
        }
        else
            fuse_reply_err( req, ret );

        fuseif_free_buf( buf );

    }while( 0 );

    return;
}


void fuseif_ll_write_buf(fuse_req_t req,
    fuse_ino_t ino, fuse_bufvec *buf,
    off_t off, fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        fuse* pFuse = GetFuse();
        fuseif_intr_data* d = new fuseif_intr_data;
        d->fe = ( CFuseObjBase* )fi->fh;
        fuseif_prepare_interrupt( pFuse, req, d );
        d->fe->fs_write_buf( req, buf, fi, d );
        fuseif_finish_interrupt( pFuse, req, d );
        delete d;
        if( SUCCEEDED( ret ) )
            fuse_reply_write(
                req, fuseif_buf_size( buf ) );
        else
            fuse_reply_err( req, ret );

    }while( 0 );

    return;
}

void fuseif_tweak_llops( fuse_session* se )
{
    if( se == nullptr )
        return;
    fuse_session_dup* ps = reinterpret_cast
        < fuse_session_dup* >(se );
    if( ps == nullptr )
        return;

    ps->op.read = fuseif_ll_read;
    ps->op.write = fuseif_ll_write;
    return;
}

