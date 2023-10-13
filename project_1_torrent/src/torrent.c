// NXC Data Communications Network torrent.c for BitTorrent-like P2P File Sharing System
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#include "torrent.h"
#include "torrent_ui.h"
#include "torrent_engine.h"
#include "torrent_utils.h"

//// MULTITHREADING FUNCTIONS ////

int torrent_client_ans (torrent_engine_t *engine);
int torrent_server_ans (torrent_engine_t *engine);
int listen_socket_ans (int port);

void *torrent_engine_thread (void *_engine)
{
    torrent_engine_t *engine = (torrent_engine_t *)_engine;
    if (engine->listen_sock == -1)
    {
        // Change this line to listen_socket() to test your implementation
        engine->listen_sock = listen_socket_ans (engine->port);
        if (engine->listen_sock == -1)
        {
            ERROR_PRTF ("ERROR torrent_engine_thread(): listen_socket_ans() failed.\n");
            exit (EXIT_FAILURE);
        }
    }
    while (engine->stop_engine == 0)
    {
        pthread_mutex_lock (&engine->mutex);

        // Change this line to torrent_client() to test your implementation
        torrent_client_ans (engine);
        size_t start_time = get_elapsed_msec();
        while (get_elapsed_msec () < start_time + SERVER_TIME_MSEC)
            // Change this line to torrent_server() to test your implementation
            torrent_server_ans (engine);

        pthread_mutex_unlock (&engine->mutex);

        usleep ((rand() % RAND_WAIT_MSEC + 10) * 1000);
        if (print_info == 1)
        {
            INFO_PRTF ("ENGINE REV COMPLETE.\n");
            print_engine_status (engine);
        }
    }
    return NULL;
}

void *request_torrent_info_thread_wrapper (void *_request_wrapper_data)
{
    request_wrapper_data_t *request_wrapper_data = (request_wrapper_data_t *)_request_wrapper_data;
    request_torrent_info (request_wrapper_data->peer, request_wrapper_data->torrent);
    free (request_wrapper_data);
    return NULL;
}

void *request_torrent_peer_list_thread_wrapper (void *_request_wrapper_data)
{
    request_wrapper_data_t *request_wrapper_data = (request_wrapper_data_t *)_request_wrapper_data;
    request_torrent_peer_list (request_wrapper_data->peer, request_wrapper_data->torrent);
    free (request_wrapper_data);
    return NULL;
}

void *request_torrent_block_status_thread_wrapper (void *_request_wrapper_data)
{
    request_wrapper_data_t *request_wrapper_data = (request_wrapper_data_t *)_request_wrapper_data;
    request_torrent_block_status (request_wrapper_data->peer, request_wrapper_data->torrent);
    free (request_wrapper_data);
    return NULL;
}

void *request_torrent_block_thread_wrapper (void *_request_wrapper_data)
{
    request_wrapper_data_t *request_wrapper_data = (request_wrapper_data_t *)_request_wrapper_data;
    request_torrent_block (request_wrapper_data->peer, request_wrapper_data->torrent, 
            request_wrapper_data->block_index);
    free (request_wrapper_data);
    return NULL;
}

int request_torrent_info_thread (peer_data_t *peer, torrent_t *torrent)
{
    pthread_t request_thread;
    pthread_attr_t attr;
    request_wrapper_data_t *request_wrapper_data = calloc (1, sizeof (request_wrapper_data_t));
    if (request_wrapper_data == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    request_wrapper_data->peer = peer;
    request_wrapper_data->torrent = torrent;
    request_wrapper_data->block_index = 0;
    if (pthread_attr_init(&attr) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): pthread_attr_init failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): pthread_attr_setdetachstate failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_create (&request_thread, &attr, request_torrent_info_thread_wrapper, 
            (void *)request_wrapper_data) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): pthread_create failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_destroy(&attr) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_info_thread(): pthread_attr_destroy failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int request_torrent_peer_list_thread (peer_data_t *peer, torrent_t *torrent)
{
    pthread_t request_thread;
    pthread_attr_t attr;
    request_wrapper_data_t *request_wrapper_data = calloc (1, sizeof (request_wrapper_data_t));
    if (request_wrapper_data == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    request_wrapper_data->peer = peer;
    request_wrapper_data->torrent = torrent;
    request_wrapper_data->block_index = 0;
    if (pthread_attr_init(&attr) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): pthread_attr_init failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): pthread_attr_setdetachstate failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_create (&request_thread, NULL, request_torrent_peer_list_thread_wrapper, 
            (void *)request_wrapper_data) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): pthread_create failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_destroy(&attr) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_peer_list_thread(): pthread_attr_destroy failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int request_torrent_block_status_thread (peer_data_t *peer, torrent_t *torrent)
{
    pthread_t request_thread;
    pthread_attr_t attr;
    request_wrapper_data_t *request_wrapper_data = calloc (1, sizeof (request_wrapper_data_t));
    if (request_wrapper_data == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    request_wrapper_data->peer = peer;
    request_wrapper_data->torrent = torrent;
    request_wrapper_data->block_index = 0;
    if (pthread_attr_init(&attr) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): pthread_attr_init failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): pthread_attr_setdetachstate failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_create (&request_thread, NULL, request_torrent_block_status_thread_wrapper, 
            (void *)request_wrapper_data) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): pthread_create failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_destroy(&attr) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_block_status_thread(): pthread_attr_destroy failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int request_torrent_block_thread (peer_data_t *peer, torrent_t *torrent, size_t block_index)
{
    pthread_t request_thread;
    pthread_attr_t attr;
    request_wrapper_data_t *request_wrapper_data = calloc (1, sizeof (request_wrapper_data_t));
    if (request_wrapper_data == NULL)
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    request_wrapper_data->peer = peer;
    request_wrapper_data->torrent = torrent;
    request_wrapper_data->block_index = block_index;
    if (pthread_attr_init(&attr) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): pthread_attr_init failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): pthread_attr_setdetachstate failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_create (&request_thread, NULL, request_torrent_block_thread_wrapper, 
            (void *)request_wrapper_data) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): pthread_create failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (pthread_attr_destroy(&attr) != 0)
    {
        ERROR_PRTF ("ERROR request_torrent_block_thread(): pthread_attr_destroy failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

//// TORRENT MANAGEMENT FUNCTIONS ////

torrent_t *init_torrent (torrent_engine_t *engine)
{
    torrent_t *torrent = calloc (1, sizeof (torrent_t));
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return NULL;
    }
    torrent->engine = engine;
    torrent->torrent_hash = 0;
    memset (torrent->torrent_name, 0, STR_LEN);
    torrent->file_size = 0;
    torrent->data = NULL;

    torrent->last_torrent_save_msec = 0;
    torrent->last_block_status_reset_msec = 0;
    torrent->num_blocks = 0;
    torrent->block_hashes = NULL;
    torrent->block_status = NULL;

    torrent->download_speed = 0;
    torrent->download_speed_prev_msec = 0;
    torrent->download_speed_prev_num_blocks = 0;

    torrent->num_peers = 0;
    torrent->max_num_peers = DEFAULT_ARR_MAX_NUM;
    torrent->peers = calloc (torrent->max_num_peers, sizeof (peer_data_t*));
    if (torrent->peers == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent(): peers calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        destroy_torrent (torrent);
        return NULL;
    }

    return torrent;
}

torrent_t *init_torrent_from_file (torrent_engine_t *engine, char *torrent_name, char* path)
{
    if (torrent_name == NULL || path == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): invalid arguments.\n");
        return NULL;
    }
    torrent_t *torrent = init_torrent (engine);
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): init_torrent() failed.\n");
        return NULL;
    }
    ssize_t file_size = get_file_size (path);
    if (file_size == -1)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): get_file_size() failed.\n");
        destroy_torrent (torrent);
        return NULL;
    }
    if (set_torrent_info (torrent, torrent_name, file_size) == -1)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): set_torrent_info() failed.\n");
        destroy_torrent (torrent);
        return NULL;
    }
    if (read_file (path, torrent->data) != torrent->file_size)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): read_file() failed.\n");
        destroy_torrent (torrent);
        return NULL;
    }

    for (size_t i = 0; i < torrent->num_blocks; i++)
    {
        torrent->block_hashes[i] = get_hash (get_block_ptr (torrent, i), BLOCK_SIZE);
        torrent->block_status[i] = B_READY;
    }
    torrent->torrent_hash = get_hash (torrent->data, torrent->file_size);
    return torrent;
}

torrent_t *init_torrent_from_hash (torrent_engine_t *engine, HASH_t torrent_hash)
{
    if (torrent_hash == 0)
    {
        ERROR_PRTF ("ERROR init_torrent_from_hash(): invalid arguments.\n");
        return NULL;
    }
    torrent_t *torrent = init_torrent (engine);
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_hash(): init_torrent() failed.\n");
        return NULL;
    }
    torrent->torrent_hash = torrent_hash;
    return torrent;
}

int set_torrent_info (torrent_t *torrent, char *torrent_name, size_t file_size)
{
    if (torrent == NULL || file_size == 0)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): invalid arguments.\n");
        return -1;
    }
    if (is_torrent_info_set (torrent) == 1)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): torrent info is already set.\n");
        return -1;
    }
    strncpy (torrent->torrent_name, torrent_name, STR_LEN);
    torrent->torrent_name[STR_LEN - 1] = '\0';
    torrent->file_size = file_size;

    torrent->num_blocks = (torrent->file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    torrent->data = calloc (torrent->num_blocks, BLOCK_SIZE);
    if (torrent->data == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): data calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    
    torrent->block_hashes = calloc (torrent->num_blocks, sizeof (HASH_t));
    if (torrent->block_hashes == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): block_hashes calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        free (torrent->data);
        torrent->data = NULL;
        return -1;
    }

    torrent->block_status = calloc (torrent->num_blocks, sizeof (B_STAT));
    if (torrent->block_status == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): block_status calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        free (torrent->data);
        free (torrent->block_hashes);
        torrent->data = NULL;
        torrent->block_hashes = NULL;
        return -1;
    }

    for (size_t i = 0; i < torrent->num_blocks; i++)
    {
        torrent->block_hashes[i] = 0;
        torrent->block_status[i] = B_MISSING;
    }

    for (size_t i = 0; i < torrent->num_peers; i++)
    {
        if (set_peer_block_info (torrent->peers[i]) == -1)
        {
            ERROR_PRTF ("ERROR init_torrent_from_file(): set_peer_block_info() failed.\n");
            free (torrent->data);
            free (torrent->block_hashes);
            free (torrent->block_status);
            torrent->data = NULL;
            torrent->block_hashes = NULL;
            torrent->block_status = NULL;
            return -1;
        }
    }

    return 0;
}

int is_torrent_info_set (torrent_t *torrent)
{
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR get_is_torrent_info_ready(): invalid arguments.\n");
        return -1;
    }
    if (torrent->data == NULL || torrent->file_size == 0 || torrent->num_blocks == 0
        || torrent->block_hashes == NULL || torrent->block_status == NULL)
    {
        if (torrent->data != NULL || torrent->file_size != 0 || torrent->num_blocks != 0
            || torrent->block_hashes != NULL || torrent->block_status != NULL)
        {
            ERROR_PRTF ("ERROR get_is_torrent_info_ready(): torrent info is in illegal state.\
            - data: %p, file size: %ld, num blocks: %ld, block hashes: %p, block status: %p\n",
                torrent->data, torrent->file_size, torrent->num_blocks, torrent->block_hashes, 
                    torrent->block_status);
            return -1;
        }
        return 0;
    }
    return 1;
}

void destroy_torrent (torrent_t *torrent)
{
    if (torrent == NULL)
        return;
    free (torrent->data);
    free (torrent->block_hashes);
    free (torrent->block_status);
    if (torrent->peers != NULL)
    {
        for (size_t i = 0; i < torrent->max_num_peers; i++)
            destroy_peer_data (torrent->peers[i]);
        free (torrent->peers);
    }
    free (torrent);
}

int save_torrent_as_file (torrent_t *torrent)
{
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR save_torrent_as_file(): invalid arguments.\n");
        return -1;
    }
    if (is_torrent_info_set (torrent) != 1)
    {
        ERROR_PRTF ("ERROR save_torrent_as_file(): torrent info is not set.\n");
        return -1;
    }

    // Update torrent save time.
    torrent->last_torrent_save_msec = get_elapsed_msec();

    char path [STR_LEN*2 + 12] = {0};
    struct stat st = {0};
    strcpy (path, SAVE_DIR);
    if (stat (path, &st) == -1)
    {
        if (mkdir (path, 0700) == -1)
        {
            ERROR_PRTF ("ERROR save_torrent_as_file(): mkdir failed. (ERRNO %d:%s)\n", errno, strerror(errno));
            return -1;
        }
    }

    sprintf (path + strlen (path), "/0x%08x/", torrent->engine->engine_hash);
    if (stat (path, &st) == -1)
    {
        if (mkdir (path, 0700) == -1)
        {
            ERROR_PRTF ("ERROR save_torrent_as_file(): mkdir failed. (ERRNO %d:%s)\n", errno, strerror(errno));
            return -1;
        }
    }

    strcat (path, torrent->torrent_name);
    INFO_PRTF ("SAVING 0x%08x TO %s.\n", torrent->torrent_hash, path);
    if (torrent->torrent_hash != get_hash (torrent->data, torrent->file_size))
        INFO_PRTF ("WARNING: SAVE OUTPUT HASH 0x%08x DOES NOT MATCH 0x%08x.\n", 
                get_hash (torrent->data, torrent->file_size), torrent->torrent_hash);

    FILE *fp = fopen (path, "wb");
    if (fp == NULL)
    {
        ERROR_PRTF ("ERROR save_torrent_as_file(): fopen failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return -1;
    }
    if (fwrite (torrent->data, 1, torrent->file_size, fp) != torrent->file_size)
    {
        ERROR_PRTF ("ERROR save_torrent_as_file(): fwrite failed. (ERRNO %d:%s)\n", errno, strerror(errno));;
        fclose (fp);
        return -1;
    }
    fclose (fp);
    return 0;
}

ssize_t find_torrent (torrent_engine_t *engine, HASH_t torrent_hash)
{
    if (engine == NULL || torrent_hash == 0)
    {
        ERROR_PRTF ("ERROR find_torrent(): invalid arguments.\n");
        return -1;
    }
    for (size_t i = 0; i < engine->num_torrents; i++)
    {
        if ((engine->torrents[i] != NULL) 
            && (engine->torrents[i]->torrent_hash == torrent_hash))
            return i;
    }
    return -1;
}

ssize_t find_torrent_name (torrent_engine_t *engine, char* name)
{
    if (engine == NULL || name == NULL)
    {
        ERROR_PRTF ("ERROR find_torrent_name(): invalid arguments.\n");
        return -1;
    }
    for (size_t i = 0; i < engine->num_torrents; i++)
    {
        if ((engine->torrents[i] != NULL) 
            && (strncmp (engine->torrents[i]->torrent_name, name, STR_LEN) == 0))
            return i;
    }
    return -1;
}

B_STAT get_block_status (torrent_t *torrent, size_t block_index)
{
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR get_block_status(): torrent is NULL.\n");
        return B_ERROR;
    }
    if (is_torrent_info_set (torrent) != 1)
    {
        ERROR_PRTF ("ERROR get_block_status(): torrent info is not set.\n");
        return B_ERROR;
    }
    if (block_index >= torrent->num_blocks || block_index < 0)
    {
        ERROR_PRTF ("ERROR get_block_status(): invalid block index.\n");
        return B_ERROR;
    }
    return torrent->block_status[block_index];
}

ssize_t get_num_completed_blocks (torrent_t *torrent)
{
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR get_num_completed_blocks(): invalid arguments.\n");
        return -1;
    }
    if (is_torrent_info_set (torrent) != 1)
        return 0;
    size_t num_completed_blocks = 0;
    for (size_t i = 0; i < torrent->num_blocks; i++)
    {
        if (get_block_status (torrent, i) == B_READY)
            num_completed_blocks++;
    }
    return num_completed_blocks;
}

ssize_t get_rand_missing_block_that_peer_has (torrent_t *torrent, peer_data_t *peer)
{
    if (torrent == NULL || peer == NULL)
    {
        ERROR_PRTF ("ERROR get_rand_missing_block_that_peer_has(): invalid arguments.\n");
        return -2;
    }
    if (is_torrent_info_set (torrent) != 1)
    {
        ERROR_PRTF ("ERROR get_rand_missing_block_that_peer_has(): torrent info is not set.\n");
        return -2;
    }

    ssize_t block_idx = rand() % torrent->num_blocks;
    ssize_t block_idx_start = block_idx;
    while (!(get_block_status (torrent, block_idx) == B_MISSING 
        && get_peer_block_status (peer, block_idx) == B_READY))
    {
        block_idx++;
        if (block_idx == torrent->num_blocks)
            block_idx = 0;
        if (block_idx == block_idx_start)
            return -1;
    }
    
    return block_idx;
}

B_STAT get_peer_block_status (peer_data_t *peer, size_t block_index)
{
    if (peer == NULL || block_index >= peer->torrent->num_blocks)
    {
        ERROR_PRTF ("ERROR get_peer_block_status(): invalid arguments.\n");
        return B_ERROR;
    }
    if (peer->block_status == NULL)
    {
        ERROR_PRTF ("ERROR get_peer_block_status(): peer block_status is NULL.\n");
        return B_ERROR;
    }
    return peer->block_status[block_index];
}

ssize_t get_peer_num_completed_blocks (peer_data_t *peer)
{
    if (peer == NULL)
    {
        ERROR_PRTF ("ERROR get_peer_num_completed_blocks(): invalid arguments.\n");
        return -1;
    }
    if (peer->block_status == NULL)
        return 0;
    size_t num_completed_blocks = 0;
    for (size_t i = 0; i < peer->torrent->num_blocks; i++)
    {
        if (peer->block_status[i] == B_READY)
            num_completed_blocks++;
    }
    return num_completed_blocks;
}

void *get_block_ptr (torrent_t *torrent, size_t block_index)
{
    if (torrent == NULL || block_index >= torrent->num_blocks)
    {
        ERROR_PRTF ("ERROR get_block_ptr(): invalid arguments.\n");
        return NULL;
    }
    if (is_torrent_info_set (torrent) != 1)
    {
        ERROR_PRTF ("ERROR get_block_ptr(): torrent info is not set.\n");
        return NULL;
    }
    return (void *)((char *)torrent->data + block_index * BLOCK_SIZE);
}

ssize_t get_torrent_download_speed (torrent_t *torrent)
{
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR get_torrent_download_speed(): invalid arguments.\n");
        return -1;
    }
    if (is_torrent_info_set (torrent) != 1)
        return 0;
    if (get_elapsed_msec () - torrent->download_speed_prev_msec > TORRENT_SPEED_MEASURE_INTERVAL_MSEC)
    {
        torrent->download_speed = ((get_num_completed_blocks(torrent) - torrent->download_speed_prev_num_blocks) 
            * 1000 * BLOCK_SIZE) / TORRENT_SPEED_MEASURE_INTERVAL_MSEC;
        torrent->download_speed_prev_msec = get_elapsed_msec ();
        torrent->download_speed_prev_num_blocks = get_num_completed_blocks(torrent);
    }
    return torrent->download_speed;
}

int update_if_max_torrent_reached (torrent_engine_t *engine)
{
    if (engine == NULL)
    {
        ERROR_PRTF ("ERROR update_if_max_torrent_reached(): invalid arguments.\n");
        return -1;
    }
    if (engine->max_num_torrents == engine->num_torrents)
    {
        engine->max_num_torrents *= 2;
        engine->torrents = realloc (engine->torrents, engine->max_num_torrents * sizeof (torrent_t*));
        if (engine->torrents == NULL)
        {
            ERROR_PRTF ("ERROR update_if_max_torrent_reached(): realloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
            return -1;
        }
        for (size_t i = engine->num_torrents; i < engine->max_num_torrents; i++)
            engine->torrents[i] = NULL;
    }
    return 0;
}

peer_data_t *init_peer_data (torrent_t *torrent)
{
    peer_data_t *peer_data = calloc (1, sizeof (peer_data_t));
    if (peer_data == NULL)
    {
        ERROR_PRTF ("ERROR init_peer_data(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return NULL;
    }
    peer_data->torrent = torrent;
    memset (peer_data->ip, 0, STR_LEN);
    peer_data->port = -1;
    peer_data->last_torrent_info_request_msec = 0;
    peer_data->last_peer_list_request_msec = 0;
    peer_data->last_block_status_request_msec = 0;
    peer_data->last_block_request_msec = 0;
    peer_data->block_status = NULL;
    return peer_data;
}

int set_peer_block_info (peer_data_t *peer_data)
{
    if (peer_data == NULL)
    {
        ERROR_PRTF ("ERROR set_peer_info(): invalid arguments.\n");
        return -1;
    }
    torrent_t *torrent = peer_data->torrent;
    if (peer_data->block_status != NULL)
    {
        ERROR_PRTF ("ERROR set_peer_info(): peer block_status is not NULL.\n");
        return -1;
    }
    if (is_torrent_info_set (torrent) != 1)
    {
        ERROR_PRTF ("ERROR set_peer_info(): torrent info is not set.\n");
        return -1;
    }

    if (peer_data->block_status != NULL)
    {
        ERROR_PRTF ("ERROR set_peer_block_info(): peer block_status is not NULL.\n");
        return -1;
    }
    peer_data->block_status = calloc (torrent->num_blocks, sizeof (B_STAT));
    if (peer_data->block_status == NULL)
    {
        ERROR_PRTF ("ERROR init_peer_data(): block_status calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        destroy_peer_data (peer_data);
        return -1;
    }
    for (size_t i = 0; i < torrent->num_blocks; i++)
        peer_data->block_status[i] = B_MISSING;
    return 0;
}

ssize_t torrent_add_peer (torrent_t *torrent, char *ip, int port)
{
    if (torrent == NULL || ip == NULL || port < 0 || port > 65535)
    {
        ERROR_PRTF ("ERROR torrent_add_peer(): invalid arguments.\n");
        return -1;
    }
    if (find_peer(torrent, ip, port) != -1)
    {
        ERROR_PRTF ("ERROR torrent_add_peer(): torrent already has the peer %s:%d\n",
            ip, port);
        return -1;
    }

    peer_data_t *peer_data = init_peer_data (torrent);
    if (peer_data == NULL)
    {
        ERROR_PRTF ("ERROR torrent_add_peer(): init_peer_data() failed.\n");
        return -1;
    }
    if (is_torrent_info_set (torrent) == 1)
    {
        if (set_peer_block_info (peer_data) == -1)
        {
            ERROR_PRTF ("ERROR torrent_add_peer(): set_peer_block_info() failed.\n");
            destroy_peer_data (peer_data);
            return -1;
        }
    }
    strncpy (peer_data->ip, ip, STR_LEN);
    peer_data->ip[STR_LEN - 1] = '\0';
    peer_data->port = port;

    ssize_t peer_idx = torrent->num_peers++;
    torrent->peers[peer_idx] = peer_data;
    update_if_max_peer_reached (torrent);

    return peer_idx;
}

int torrent_remove_peer (torrent_t *torrent, char *ip, int port)
{
    if (torrent == NULL || ip == NULL || port < 0 || port > 65535)
    {
        ERROR_PRTF ("ERROR torrent_remove_peer(): invalid arguments.\n");
        return -1;
    }
    ssize_t peer_idx = find_peer (torrent, ip, port);
    if (peer_idx == -1)
        return 0;
    peer_data_t *peer = torrent->peers[peer_idx];
    
    if (peer_idx == torrent->num_peers - 1)
    {
        torrent->peers[peer_idx] = NULL;
    }
    else
    {
        torrent->peers[peer_idx] = torrent->peers[torrent->num_peers - 1];
        torrent->peers[torrent->num_peers - 1] = NULL;
    }
    torrent->num_peers--;
    destroy_peer_data (peer);
    return 0;
}

void destroy_peer_data (peer_data_t *peer_data)
{
    if (peer_data == NULL)
        return;
    free (peer_data->block_status);
    free (peer_data);
}

ssize_t find_peer (torrent_t *torrent, char *ip, int port)
{
    if (torrent == NULL || ip == NULL || port < 0 || port > 65535)
    {
        ERROR_PRTF ("ERROR find_peer(): invalid arguments.\n");
        return -1;
    }
    for (size_t i = 0; i < torrent->num_peers; i++)
    {
        if ((torrent->peers[i] != NULL) 
            && (strncmp (torrent->peers[i]->ip, ip, STR_LEN) == 0)
            && (torrent->peers[i]->port == port))
            return i;
    }
    return -1;
}   

int update_if_max_peer_reached (torrent_t *torrent)
{
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR update_if_max_peer_reached(): invalid arguments.\n");
        return -1;
    }
    if (torrent->num_peers == torrent->max_num_peers)
    {
        torrent->max_num_peers *= 2;
        torrent->peers = realloc (torrent->peers, torrent->max_num_peers * sizeof (peer_data_t*));
        if (torrent->peers == NULL)
        {
            ERROR_PRTF ("ERROR update_if_max_peer_reached(): realloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
            return -1;
        }
        for (size_t i = torrent->num_peers; i < torrent->max_num_peers; i++)
            torrent->peers[i] = NULL;
    }
    return 0;
}
