// NXC Data Communications torrent.c for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#include "torrent.h"
#include "torrent_ui.h"
#include "torrent_engine.h"
#include "torrent_utils.h"

//// TORRENT MANAGEMENT FUNCTIONS ////

torrent_t *init_torrent (torrent_engine_t *engine)
{
    torrent_t *torrent = calloc (1, sizeof (torrent_t));
    if (torrent == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent(): calloc failed.\n");
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

    torrent->num_peers = 0;
    torrent->max_num_peers = DEFAULT_ARR_MAX_NUM;
    torrent->peers = calloc (torrent->max_num_peers, sizeof (peer_data_t*));
    if (torrent->peers == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent(): peers calloc failed.\n");
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
    if (torrent->data != NULL || torrent->file_size != 0 
        || torrent->num_blocks != 0 || torrent->block_status != NULL || torrent->block_hashes != NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): torrent data is not empty.\n");
        return -1;
    }
    strncpy (torrent->torrent_name, torrent_name, STR_LEN);
    torrent->torrent_name[STR_LEN - 1] = '\0';
    torrent->file_size = file_size;

    torrent->num_blocks = (torrent->file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    torrent->data = calloc (torrent->num_blocks, BLOCK_SIZE);
    if (torrent->data == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): data calloc failed.\n");
        return -1;
    }
    
    torrent->block_hashes = calloc (torrent->num_blocks, sizeof (HASH_t));
    if (torrent->block_hashes == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): block_hashes calloc failed.\n");
        free (torrent->data);
        torrent->data = NULL;
        return -1;
    }

    torrent->block_status = calloc (torrent->num_blocks, sizeof (B_STAT));
    if (torrent->block_status == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_from_file(): block_status calloc failed.\n");
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
    if (torrent->data == NULL || torrent->file_size == 0)
    {
        ERROR_PRTF ("ERROR save_torrent_as_file(): torrent data is empty.\n");
        return -1;
    }

    // Update torrent save time.
    torrent->last_torrent_save_msec = get_time_msec();

    char path [STR_LEN*2 + 12] = {0};
    struct stat st = {0};
    strcpy (path, SAVE_DIR);
    if (stat (path, &st) == -1)
    {
        if (mkdir (path, 0700) == -1)
        {
            ERROR_PRTF ("ERROR save_torrent_as_file(): mkdir failed.\n");
            return -1;
        }
    }

    sprintf (path + strlen (path), "/0x%08x/", torrent->engine->engine_hash);
    if (stat (path, &st) == -1)
    {
        if (mkdir (path, 0700) == -1)
        {
            ERROR_PRTF ("ERROR save_torrent_as_file(): mkdir failed.\n");
            return -1;
        }
    }

    strcat (path, torrent->torrent_name);
    INFO_PRTF ("\t[%04.3fs] SAVING 0x%08x TO %s.\n", (double)get_elapsed_msec()/1000, 
            torrent->torrent_hash, path);
    if (torrent->torrent_hash != get_hash (torrent->data, torrent->file_size))
        INFO_PRTF ("\t[%04.3fs] WARNING: SAVE OUTPUT HASH 0x%08x DOES NOT MATCH 0x%08x.\n", 
                (double)get_elapsed_msec()/1000, 
                    get_hash (torrent->data, torrent->file_size), torrent->torrent_hash);

    FILE *fp = fopen (path, "wb");
    if (fp == NULL)
    {
        ERROR_PRTF ("ERROR save_torrent_as_file(): fopen failed.\n");
        return -1;
    }
    if (fwrite (torrent->data, 1, torrent->file_size, fp) != torrent->file_size)
    {
        ERROR_PRTF ("ERROR save_torrent_as_file(): fwrite failed.\n");
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
    if (torrent->block_status == NULL)
    {
        ERROR_PRTF ("ERROR get_block_status(): torrent block_status is NULL.\n");
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
    if (torrent->block_status == NULL)
        return 0;
    size_t num_completed_blocks = 0;
    for (size_t i = 0; i < torrent->num_blocks; i++)
    {
        if (get_block_status (torrent, i) == B_READY)
            num_completed_blocks++;
    }
    return num_completed_blocks;
}

ssize_t get_missing_block (torrent_t *torrent, size_t start_idx)
{
    if (torrent == NULL || start_idx >= torrent->num_blocks)
    {
        ERROR_PRTF ("ERROR get_missing_block(): invalid arguments. - torrent: %p, %ld\n", torrent, start_idx);
        return -2;
    }
    if (torrent->block_status == NULL)
    {
        ERROR_PRTF ("ERROR get_missing_block(): torrent block_status is NULL.\n");
        return -2;
    }
    for (size_t i = start_idx; i < torrent->num_blocks; i++)
    {
        if (get_block_status (torrent, i) == B_MISSING)
            return i;
    }
    return -1;
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
    if (torrent->data == NULL)
    {
        ERROR_PRTF ("ERROR get_block_ptr(): torrent data is NULL.\n");
        return NULL;
    }
    return (void *)((char *)torrent->data + block_index * BLOCK_SIZE);
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
            ERROR_PRTF ("ERROR update_if_max_torrent_reached(): realloc failed.\n");
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
        ERROR_PRTF ("ERROR init_peer_data(): calloc failed.\n");
        return NULL;
    }
    peer_data->torrent = torrent;
    memset (peer_data->ip, 0, STR_LEN);
    peer_data->port = -1;
    peer_data->num_requests = 0;
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
    if (torrent->num_blocks == 0)
    {
        ERROR_PRTF ("ERROR set_peer_info(): torrent num_blocks is 0.\n");
        return -1;
    }

    peer_data->block_status = calloc (torrent->num_blocks, sizeof (B_STAT));
    if (peer_data->block_status == NULL)
    {
        ERROR_PRTF ("ERROR init_peer_data(): block_status calloc failed.\n");
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
    if (torrent->num_blocks > 0)
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
            ERROR_PRTF ("ERROR update_if_max_peer_reached(): realloc failed.\n");
            return -1;
        }
        for (size_t i = torrent->num_peers; i < torrent->max_num_peers; i++)
            torrent->peers[i] = NULL;
    }
    return 0;
}
