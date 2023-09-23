// NXC Data Communications torrent_ui.c for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#include "torrent_ui.h"
#include "torrent_engine.h"
#include "torrent_utils.h"

//// UI ELEMENTS ////  

void *torrent_engine_thread (void *_engine)
{
    torrent_engine_t *engine = (torrent_engine_t *)_engine;
    while (engine->stop_engine == 0)
    {
        pthread_mutex_lock (&engine->mutex);
        torrent_client (engine);
        torrent_server (engine);
        pthread_mutex_unlock (&engine->mutex);
        usleep ((rand() % RAND_WAIT_RANGE) * 1000);
    }
    return NULL;
}

torrent_engine_t *init_torrent_engine (int port)
{
    if (port < 0 || port > 65535)
    {
        ERROR_PRTF ("ERROR init_torrent_engine(): invalid port number.\n");
        return NULL;
    }
    torrent_engine_t *engine = calloc (1, sizeof (torrent_engine_t));
    if (engine == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_engine(): calloc failed.\n");
        return NULL;
    }
    engine->port = port;
    engine->listen_sock = listen_socket (port);
    if (engine->listen_sock == -1)
    {
        ERROR_PRTF ("ERROR init_torrent_engine(): listen_socket() failed.\n");
        free (engine);
        return NULL;
    }
    size_t unique_val = getpid() + get_time_msec();
    engine->engine_hash = get_hash (&unique_val, sizeof (size_t));

    engine->num_torrents = 0;
    engine->max_num_torrents = DEFAULT_ARR_MAX_NUM;
    engine->torrents = calloc (engine->max_num_torrents, sizeof (torrent_t*));
    if (engine->torrents == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_engine(): torrents calloc failed.\n");
        destroy_torrent_engine (engine);
        return NULL;
    }
    for (size_t i = 0; i < engine->max_num_torrents; i++)
        engine->torrents[i] = NULL;

    engine->stop_engine = 0;
    pthread_mutex_init (&engine->mutex, NULL);
    pthread_create (&engine->thread, NULL, torrent_engine_thread, engine);

    return engine;
}

void destroy_torrent_engine (torrent_engine_t *engine)
{
    if (engine == NULL)
        return;
    engine->stop_engine = 1;
    pthread_join (engine->thread, NULL);
    if (engine->torrents != NULL)
    {
        for (size_t i = 0; i < engine->max_num_torrents; i++)
            destroy_torrent (engine->torrents[i]);
        free (engine->torrents);
    }
    pthread_mutex_destroy (&engine->mutex);
    free (engine);
}

HASH_t create_new_torrent (torrent_engine_t *engine, char *torrent_name, char *path)
{
    if (engine == NULL || torrent_name == NULL || path == NULL)
    {
        ERROR_PRTF ("ERROR create_new_torrent(): invalid arguments.\n");
        return 0;
    }
    torrent_t *torrent = init_torrent_from_file (engine, torrent_name, path);
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR create_new_torrent(): init_torrent_from_file() failed.\n");
        return 0;
    }
    if (find_torrent (engine, torrent->torrent_hash) != -1)
    {
        ERROR_PRTF ("ERROR create_new_torrent(): engine already has the torrent 0x%08x\n"
            , torrent->torrent_hash);
        destroy_torrent (torrent);
        return 0;
    }
    engine->torrents[engine->num_torrents++] = torrent;
    update_if_max_torrent_reached (engine);
    save_torrent_as_file (torrent);
    return torrent->torrent_hash;
}

int add_torrent (torrent_engine_t *engine, HASH_t torrent_hash)
{
    if (engine == NULL || torrent_hash == 0)
    {
        ERROR_PRTF ("ERROR add_torrent(): invalid arguments.\n");
        return -1;
    }
    ssize_t torrent_idx = find_torrent (engine, torrent_hash);
    if (torrent_idx != -1)
    {
        ERROR_PRTF ("ERROR add_torrent(): engine already has the torrent 0x%08x\n",
            torrent_hash);
        return -1;
    }
    torrent_t *torrent = init_torrent_from_hash (engine, torrent_hash);
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR add_torrent(): init_torrent_from_hash() failed.\n");
        return -1;
    }
    engine->torrents[engine->num_torrents++] = torrent;
    update_if_max_torrent_reached (engine);
    return 0;
}

int remove_torrent (torrent_engine_t *engine, HASH_t torrent_hash)
{
    if (engine == NULL || torrent_hash == 0)
    {
        ERROR_PRTF ("ERROR remove_torrent(): invalid arguments.\n");
        return -1;
    }
    ssize_t torrent_idx = find_torrent (engine, torrent_hash);
    if (torrent_idx == -1)
        return 0;
    torrent_t *torrent = engine->torrents[torrent_idx];
    if (torrent_idx == engine->num_torrents - 1)
    {
        engine->torrents[torrent_idx] = NULL;
    }
    else
    {
        engine->torrents[torrent_idx] = engine->torrents[engine->num_torrents - 1];
        engine->torrents[engine->num_torrents - 1] = NULL;
    }
    engine->num_torrents--;
    destroy_torrent (torrent);
    return 0;
}

int add_peer (torrent_engine_t *engine, HASH_t torrent_hash, char *ip, int port)
{
    if (engine == NULL || torrent_hash == 0 || ip == NULL || port < 0 || port > 65535)
    {
        ERROR_PRTF ("ERROR remove_torrent(): invalid arguments.\n");
        return -1;
    }
    ssize_t torrent_idx = find_torrent (engine, torrent_hash);
    if (torrent_idx == -1)
    {
        ERROR_PRTF ("ERROR add_peer(): engine does not have the torrent 0x%08x\n",
            torrent_hash);
        return -1;
    }
    return torrent_add_peer (engine->torrents[torrent_idx], ip, port);
}

int remove_peer (torrent_engine_t *engine, HASH_t torrent_hash, char *ip, int port)
{
    if (engine == NULL || torrent_hash == 0 || ip == NULL || port < 0 || port > 65535)
    {
        ERROR_PRTF ("ERROR remove_torrent(): invalid arguments.\n");
        return -1;
    }
    ssize_t torrent_idx = find_torrent (engine, torrent_hash);
    if (torrent_idx == -1)
    {
        ERROR_PRTF ("ERROR remove_peer(): engine does not have the torrent 0x%08x\n",
            torrent_hash);
        return -1;
    }
    return torrent_remove_peer (engine->torrents[torrent_idx], ip, port);
}

void print_engine_status (torrent_engine_t *engine)
{
    if (engine == NULL)
        return;
    if (engine->stop_engine == 0)
        GREEN_PRTF ("\tENGINE RUNNING - ")
    else
        RED_PRTF ("\tENGINE KILLED - ")
    printf ("ENGINE HASH 0x%08x, PORT: %d, NUM_TORRENTS: %ld\n", engine->engine_hash,
        engine->port, engine->num_torrents);
    for (size_t i = 0; i < engine->num_torrents; i++)
    {
        YELLOW_PRTF ("\tTORRENT %ld:\n", i);
        torrent_t *torrent = engine->torrents[i];
        printf ("\t\tNAME: %s\n", torrent->torrent_name? torrent->torrent_name : "NULL");
        printf ("\t\tHASH: 0x%08x, SIZE: %ld, BLOCKS: %ld/%ld, NUM PEERS: %ld\n",
            torrent->torrent_hash, torrent->file_size, 
                get_num_completed_blocks(torrent), torrent->num_blocks, torrent->num_peers);
        if (torrent->num_peers != 0)
        {
            printf ("\t\t\tPEERS:");
            for (size_t j = 0; j < torrent->num_peers; j++)
            {
                printf ("%s:%d (%ld/%ld) ", torrent->peers[j]->ip, torrent->peers[j]->port,
                    get_peer_num_completed_blocks (torrent->peers[j]), torrent->num_blocks);
            }
        }
    }
}

void print_torrent_status (torrent_t *torrent)
{
    if (torrent == NULL)
        return;
    printf ("\tNAME: %s\n", torrent->torrent_name? torrent->torrent_name : "NULL");
    printf ("\tHASH: 0x%08x, SIZE: %ld, BLOCKS: %ld/%ld, NUM PEERS: %ld\n",
        torrent->torrent_hash, torrent->file_size, 
            get_num_completed_blocks(torrent), torrent->num_blocks, torrent->num_peers);
    if (torrent->num_blocks != 0)
    {
        printf ("\t\tBLOCK STATUS:");
        int leading_space_num = get_int_str_len (torrent->num_blocks);
        for (size_t i = 0; i < torrent->num_blocks; i++)
        {
            if (i % PRINT_COL_NUM == 0)
                printf ("\n\t\t\t%*ld ", leading_space_num, i);
            printf ("%*d ", leading_space_num, torrent->block_status[i]);
        }
        printf ("\n");
    }
    if (torrent->num_peers != 0)
    {
        printf ("\tPEER STATUS:\n");
        for (size_t i = 0; i < torrent->num_peers; i++)
        {
            YELLOW_PRTF ("\t\tPEER %ld:\n", i);
            print_peer_status (torrent->peers[i]);
        }
    }
    printf ("\n");
}

void print_torrent_status_hash (torrent_engine_t *engine, HASH_t torrent_hash)
{
    if (torrent_hash == 0)
        return;
    size_t idx = find_torrent (engine, torrent_hash);
    if (idx == -1)
    {
        ERROR_PRTF ("ERROR print_torrent_status_hash(): engine does not have the torrent 0x%08x\n",
            torrent_hash);
        return;
    }
    print_torrent_status (engine->torrents[idx]);
}

void print_peer_status (peer_data_t *peer)
{
    if (peer == NULL)
        return;
    printf ("\t\t\tIP: %s, PORT: %d, NUM REQUESTS: %ld\n", 
        peer->ip, peer->port, peer->num_requests);
    if (peer->torrent->num_blocks != 0)
    {
        printf ("\t\t\tPEER BLOCK STATUS:");
        int leading_space_num = get_int_str_len (peer->torrent->num_blocks);
        for (size_t i = 0; i < peer->torrent->num_blocks; i++)
        {
            if (i % PRINT_COL_NUM == 0)
                printf ("\n\t\t\t%*ld ", leading_space_num, i);
            printf ("%*d ", leading_space_num, peer->block_status[i]);
        }
        printf ("\n");
    }
}
