// NXC Data Communications torrent_util.c for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////

#include "torrent_functions.h"

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
    }
    return NULL;
}

torrent_engine_t *init_torrent_engine (int port)
{
    if (port < 0 || port > 65535)
    {
        printf ("Error init_torrent_engine(): invalid port number.\n");
        return NULL;
    }
    torrent_engine_t *engine = calloc (1, sizeof (torrent_engine_t));
    if (engine == NULL)
    {
        printf ("Error init_torrent_engine(): calloc failed.\n");
        return NULL;
    }
    engine->port = port;

    engine->num_torrents = 0;
    engine->max_num_torrents = DEFAULT_MAX_NUM;
    engine->torrents = calloc (engine->max_num_torrents, sizeof (torrent_t*));
    if (engine->torrents == NULL)
    {
        printf ("Error init_torrent_engine(): torrents calloc failed.\n");
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

int create_new_torrent (torrent_engine_t *engine, char *torrent_name, char *path)
{
    if (engine == NULL || torrent_name == NULL || path == NULL)
    {
        printf ("Error create_new_torrent(): invalid arguments.\n");
        return -1;
    }
    torrent_t *torrent = init_torrent_from_file (torrent_name, path);
    if (torrent == NULL)
    {
        printf ("Error create_new_torrent(): init_torrent_from_file() failed.\n");
        return -1;
    }
    if (find_torrent (engine, torrent->torrent_hash) != -1)
    {
        printf ("Error create_new_torrent(): engine already has the torrent %x\n"
            , torrent->torrent_hash);
        destroy_torrent (torrent);
        return -1;
    }
    engine->torrents[engine->num_torrents++] = torrent;
    update_if_max_torrent_reached (engine);
    return 0;
}

int add_torrent (torrent_engine_t *engine, HASH_t torrent_hash)
{
    if (engine == NULL || torrent_hash == 0)
    {
        printf ("Error add_torrent(): invalid arguments.\n");
        return -1;
    }
    ssize_t torrent_idx = find_torrent (engine, torrent_hash);
    if (torrent_idx != -1)
    {
        printf ("Error add_torrent(): engine already has the torrent %x\n",
            torrent_hash);
        return -1;
    }
    engine->torrents[engine->num_torrents++] = init_torrent_from_hash (torrent_hash);
    update_if_max_torrent_reached (engine);
    return 0;
}

int remove_torrent (torrent_engine_t *engine, HASH_t torrent_hash)
{
    if (engine == NULL || torrent_hash == 0)
    {
        printf ("Error remove_torrent(): invalid arguments.\n");
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
        printf ("Error remove_torrent(): invalid arguments.\n");
        return -1;
    }
    ssize_t torrent_idx = find_torrent (engine, torrent_hash);
    if (torrent_idx == -1)
    {
        printf ("Error add_peer(): engine does not have the torrent %x\n",
            torrent_hash);
        return -1;
    }
    return torrent_add_peer (engine->torrents[torrent_idx], ip, port);
}

int remove_peer (torrent_engine_t *engine, HASH_t torrent_hash, char *ip, int port)
{
    if (engine == NULL || torrent_hash == 0 || ip == NULL || port < 0 || port > 65535)
    {
        printf ("Error remove_torrent(): invalid arguments.\n");
        return -1;
    }
    ssize_t torrent_idx = find_torrent (engine, torrent_hash);
    if (torrent_idx == -1)
    {
        printf ("Error remove_peer(): engine does not have the torrent %x\n",
            torrent_hash);
        return -1;
    }
    return torrent_remove_peer (engine->torrents[torrent_idx], ip, port);
}

void print_engine_status (torrent_engine_t *engine)
{
    if (engine == NULL)
        return;
    printf ("PRINTING ENGINE STATUS:\n");
    printf ("PORT: %d, NUM_TORRENTS: %ld\n", engine->port , engine->num_torrents);
    for (size_t i = 0; i < engine->num_torrents; i++)
    {
        printf ("\tTORRENT %ld:\n", i);
        print_torrent_status (engine->torrents[i]);
    }
}

void print_torrent_status (torrent_t *torrent)
{
    if (torrent == NULL)
        return;
    printf ("\t\tNAME: %s\n", torrent->torrent_name? torrent->torrent_name : "NULL");
    printf ("\t\tHASH: %x, SIZE: %ld, BLOCKS: %ld/%ld, NUM PEERS: %ld\n",
        torrent->torrent_hash, torrent->file_size, 
            get_num_completed_blocks(torrent), torrent->num_blocks, torrent->num_peers);
    printf ("\t\t\tBLOCK STATUS:\n\t\t\t");
    int leading_space_num = get_int_str_len (torrent->num_blocks);
    for (size_t i = 0; i < torrent->num_blocks; i++)
        printf ("%*ld ", leading_space_num, i);
    printf ("\n\t\t\t");
    for (size_t i = 0; i < torrent->num_blocks; i++)
        printf ("%*d ", leading_space_num, torrent->block_status[i]);
    for (size_t i = 0; i < torrent->num_peers; i++)
    {
        printf ("\t\tPEER %ld:\n", i);
        print_peer_status (torrent->peers[i]);
    }
}

void print_peer_status (peer_data_t *peer)
{
    if (peer == NULL)
        return;
    printf ("\t\t\tIP: %s, PORT: %d, NUM REQUESTS: %ld\n", 
        peer->ip, peer->port, peer->num_requests);
    printf ("\t\t\tBLOCK STATUS:\n\t\t\t");
    int leading_space_num = get_int_str_len (peer->torrent->num_blocks);
    for (size_t i = 0; i < peer->torrent->num_blocks; i++)
        printf ("%*ld ", leading_space_num, i);
    printf ("\n\t\t\t");
    for (size_t i = 0; i < peer->torrent->num_blocks; i++)
        printf ("%*d ", leading_space_num, peer->block_status[i]);
}

//// TORRENT MANAGEMENT FUNCTIONS ////

torrent_t *init_torrent ()
{
    torrent_t *torrent = calloc (1, sizeof (torrent_t));
    if (torrent == NULL)
    {
        printf ("Error init_torrent(): calloc failed.\n");
        return NULL;
    }
    torrent->torrent_hash = 0;
    memset (torrent->torrent_name, 0, STR_LEN);
    torrent->file_size = 0;
    torrent->data = NULL;

    torrent->num_blocks = 0;
    torrent->block_hashes = NULL;
    torrent->block_status = NULL;

    torrent->num_peers = 0;
    torrent->max_num_peers = DEFAULT_MAX_NUM;
    torrent->peers = calloc (torrent->max_num_peers, sizeof (peer_data_t*));
    if (torrent->peers == NULL)
    {
        printf ("Error init_torrent(): peers calloc failed.\n");
        destroy_torrent (torrent);
        return NULL;
    }

    return torrent;
}

torrent_t *init_torrent_from_file (char *torrent_name, char* path)
{
    if (torrent_name == NULL || path == NULL)
    {
        printf ("Error init_torrent_from_file(): invalid arguments.\n");
        return NULL;
    }
    torrent_t *torrent = init_torrent ();
    if (torrent == NULL)
    {
        printf ("Error init_torrent_from_file(): init_torrent() failed.\n");
        return NULL;
    }
    ssize_t file_size = get_file_size (path);
    if (file_size == -1)
    {
        printf ("Error init_torrent_from_file(): get_file_size() failed.\n");
        destroy_torrent (torrent);
        return NULL;
    }
    if (set_torrent_info (torrent, torrent_name, file_size) == -1)
    {
        printf ("Error init_torrent_from_file(): set_torrent_info() failed.\n");
        destroy_torrent (torrent);
        return NULL;
    }
    if (read_file (path, torrent->data) != torrent->file_size)
    {
        printf ("Error init_torrent_from_file(): read_file() failed.\n");
        destroy_torrent (torrent);
        return NULL;
    }

    for (size_t i = 0; i < torrent->num_blocks; i++)
    {
        torrent->block_hashes[i] = get_hash (get_block_ptr (torrent, i), BLOCK_SIZE);
        torrent->block_status[i] = 1;
    }
    torrent->torrent_hash = get_hash (torrent->data, torrent->file_size);
    return torrent;
}

torrent_t *init_torrent_from_hash (HASH_t torrent_hash)
{
    if (torrent_hash == -1)
    {
        printf ("Error init_torrent_from_hash(): invalid arguments.\n");
        return NULL;
    }
    torrent_t *torrent = init_torrent ();
    if (torrent == NULL)
    {
        printf ("Error init_torrent_from_hash(): init_torrent() failed.\n");
        return NULL;
    }
    torrent->torrent_hash = torrent_hash;
    return torrent;
}

int set_torrent_info (torrent_t *torrent, char *torrent_name, size_t file_size)
{
    if (torrent == NULL || file_size == 0)
    {
        printf ("Error init_torrent_from_file(): invalid arguments.\n");
        return -1;
    }
    if (torrent->data != NULL || torrent->file_size != 0 || torrent->num_blocks != 0)
    {
        printf ("Error init_torrent_from_file(): torrent data is not empty.\n");
        return -1;
    }
    strncpy (torrent->torrent_name, torrent_name, STR_LEN);
    torrent->torrent_name[STR_LEN - 1] = '\0';
    torrent->file_size = file_size;

    torrent->num_blocks = (torrent->file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    torrent->data = calloc (torrent->num_blocks, BLOCK_SIZE);
    
    torrent->block_hashes = calloc (torrent->num_blocks, sizeof (HASH_t));
    if (torrent->block_hashes == NULL)
    {
        printf ("Error init_torrent_from_file(): block_hashes calloc failed.\n");
        destroy_torrent (torrent);
        return -1;
    }
    torrent->block_status = calloc (torrent->num_blocks, sizeof(char));
    if (torrent->block_status == NULL)
    {
        printf ("Error init_torrent_from_file(): block_status calloc failed.\n");
        destroy_torrent (torrent);
        return -1;
    }

    for (size_t i = 0; i < torrent->num_blocks; i++)
    {
        torrent->block_hashes[i] = 0;
        torrent->block_status[i] = 0;
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
        printf ("Error save_torrent_as_file(): invalid arguments.\n");
        return -1;
    }
    if (torrent->data == NULL || torrent->file_size == 0)
    {
        printf ("Error save_torrent_as_file(): torrent data is empty.\n");
        return -1;
    }
    if (torrent->torrent_hash != get_hash (torrent->data, torrent->file_size))
        printf ("Warning save_torrent_as_file(): torrent hash is invalid.\n");
        
    char path [STR_LEN*2] = {0};
    strcpy (path, SAVE_DIR);
    strcat (path, torrent->torrent_name);
    FILE *fp = fopen (path, "wb");
    if (fp == NULL)
    {
        printf ("Error save_torrent_as_file(): fopen failed.\n");
        return -1;
    }
    if (fwrite (torrent->data, 1, torrent->file_size, fp) != torrent->file_size)
    {
        printf ("Error save_torrent_as_file(): fwrite failed.\n");
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
        printf ("Error find_torrent(): invalid arguments.\n");
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

ssize_t get_num_completed_blocks (torrent_t *torrent)
{
    if (torrent == NULL)
    {
        printf ("Error get_num_completed_blocks(): invalid arguments.\n");
        return -1;
    }
    if (torrent->block_status == NULL)
    {
        printf ("Error get_num_completed_blocks(): torrent block_status is NULL.\n");
        return -1;
    }
    size_t num_completed_blocks = 0;
    for (size_t i = 0; i < torrent->num_blocks; i++)
    {
        if (torrent->block_status[i] == 1)
            num_completed_blocks++;
    }
    return num_completed_blocks;
}

ssize_t get_missing_block (torrent_t *torrent, size_t start_idx)
{
    if (torrent == NULL || start_idx >= torrent->num_blocks)
    {
        printf ("Error get_missing_block(): invalid arguments.\n");
        return -1;
    }
    if (torrent->block_status == NULL)
    {
        printf ("Error get_missing_block(): torrent block_status is NULL.\n");
        return -1;
    }
    for (size_t i = start_idx; i < torrent->num_blocks; i++)
    {
        if (torrent->block_status[i] == 0)
            return i;
    }
    return -1;
}

char get_peer_block_status (peer_data_t *peer, size_t block_index)
{
    if (peer == NULL || block_index >= peer->torrent->num_blocks)
    {
        printf ("Error get_peer_block_status(): invalid arguments.\n");
        return -1;
    }
    if (peer->block_status == NULL)
    {
        printf ("Error get_peer_block_status(): peer block_status is NULL.\n");
        return -1;
    }
    return peer->block_status[block_index];
}

void *get_block_ptr (torrent_t *torrent, size_t block_index)
{
    if (torrent == NULL || block_index >= torrent->num_blocks)
    {
        printf ("Error get_block_ptr(): invalid arguments.\n");
        return NULL;
    }
    if (torrent->data == NULL)
    {
        printf ("Error get_block_ptr(): torrent data is NULL.\n");
        return NULL;
    }
    return (void *)((char *)torrent->data + block_index * BLOCK_SIZE);
}

int update_if_max_torrent_reached (torrent_engine_t *engine)
{
    if (engine == NULL)
    {
        printf ("Error update_if_max_torrent_reached(): invalid arguments.\n");
        return -1;
    }
    if (engine->max_num_torrents == engine->num_torrents)
    {
        engine->max_num_torrents *= 2;
        engine->torrents = realloc (engine->torrents, engine->max_num_torrents * sizeof (torrent_t*));
        if (engine->torrents == NULL)
        {
            printf ("Error update_if_max_torrent_reached(): realloc failed.\n");
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
        printf ("Error init_peer_data(): calloc failed.\n");
        return NULL;
    }
    peer_data->torrent = torrent;
    memset (peer_data->ip, 0, STR_LEN);
    peer_data->port = -1;
    peer_data->num_requests = 0;
    peer_data->block_status = NULL;
    return peer_data;
}

int torrent_add_peer (torrent_t *torrent, char *ip, int port)
{
    if (torrent == NULL || ip == NULL || port < 0 || port > 65535)
    {
        printf ("Error torrent_add_peer(): invalid arguments.\n");
        return -1;
    }
    if (find_peer(torrent, ip, port) != -1)
    {
        printf ("Error torrent_add_peer(): torrent already has the peer %s:%d\n",
            ip, port);
        return -1;
    }

    peer_data_t *peer_data = init_peer_data (torrent);
    if (peer_data == NULL)
    {
        printf ("Error torrent_add_peer(): init_peer_data() failed.\n");
        return -1;
    }
    strncpy (peer_data->ip, ip, STR_LEN);
    peer_data->ip[STR_LEN - 1] = '\0';
    peer_data->port = port;

    peer_data->block_status = calloc (torrent->num_blocks, sizeof (char));
    if (peer_data->block_status == NULL)
    {
        printf ("Error torrent_add_peer(): block_status calloc failed.\n");
        destroy_peer_data (peer_data);
        return -1;
    }
    for (size_t i = 0; i < torrent->num_blocks; i++)
        peer_data->block_status[i] = 0;

    torrent->peers[torrent->num_peers++] = peer_data;
    update_if_max_peer_reached (torrent);

    return 0;
}

int torrent_remove_peer (torrent_t *torrent, char *ip, int port)
{
    if (torrent == NULL || ip == NULL || port < 0 || port > 65535)
    {
        printf ("Error torrent_remove_peer(): invalid arguments.\n");
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
        printf ("Error find_peer(): invalid arguments.\n");
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
        printf ("Error update_if_max_peer_reached(): invalid arguments.\n");
        return -1;
    }
    if (torrent->num_peers == torrent->max_num_peers)
    {
        torrent->max_num_peers *= 2;
        torrent->peers = realloc (torrent->peers, torrent->max_num_peers * sizeof (peer_data_t*));
        if (torrent->peers == NULL)
        {
            printf ("Error update_if_max_peer_reached(): realloc failed.\n");
            return -1;
        }
        for (size_t i = torrent->num_peers; i < torrent->max_num_peers; i++)
            torrent->peers[i] = NULL;
    }
    return 0;
}

//// UTILITY FUNCTIONS ////

HASH_t get_hash (void* data, size_t len)
{
    if (data == NULL || len == 0)
    {
        printf ("Error get_hash(): invalid arguments.\n");
        return 0;
    }
    HASH_t hash = HASH_SEED;
    size_t i = 0;
    // XORshift 
    for (; i < len - sizeof(HASH_t); i += sizeof(HASH_t))
    {
        hash ^= *(HASH_t *)((char*)data + i);
        hash ^= hash << 13;
        hash ^= hash >> 17;
        hash ^= hash << 5;
    }
    for (; i < len; i++)
    {
        hash ^= *(uint8_t *)((char*)data + i);
        hash ^= hash << 13;
        hash ^= hash >> 17;
        hash ^= hash << 5;
    }
    // Hash of value 0 is reserved for invalid hash state.
    if (hash == 0)
        hash = get_hash (&hash, sizeof (HASH_t));

    return hash;
}

size_t get_time_msec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000ULL) + (tv.tv_usec/1000ULL);
}

int get_int_str_len (size_t num)
{
    int len = 0;
    while (num > 0)
    {
        num /= 10;
        len++;
    }
    return len;
}

ssize_t read_file (char *path, void *data)
{
    if (path == NULL || data == NULL)
    {
        printf ("Error read_file() invalid arguments.\n");
        return -1;
    }

    FILE *fp = fopen (path, "rb");
    if (fp == NULL)
    {
        printf ("Error read_file() fopen failed.\n");
        return -1;
    }
        
    fseek (fp, 0, SEEK_END);
    size_t len = ftell (fp);
    fseek (fp, 0, SEEK_SET);

    if (fread (data, 1, len, fp) != len)
    {
        printf ("Error read_file() fread failed.\n");
        fclose (fp);
        return -1;
    }
    fclose (fp);

    return len;
}

ssize_t get_file_size (char *path)
{
    FILE *fp = fopen (path, "rb");
    if (fp == NULL)
    {
        printf ("Error read_file() fopen failed.\n");
        return -1;
    }

    fseek (fp, 0, SEEK_END);
    size_t len = ftell (fp);
    fclose (fp);

    return len;
}

ssize_t write_bytes (int socket, char *buffer, size_t size)
{
    ssize_t bytes_sent = 0;
    ssize_t bytes_remaining = size;
    signal(SIGPIPE, SIG_IGN);
    while (bytes_remaining > 0)
    {
        bytes_sent = write(socket, buffer, bytes_remaining);
        if (bytes_sent == -1)
        {
            printf("ERROR - write() error\n");
            return -1;
        }
        bytes_remaining -= bytes_sent;
        buffer += bytes_sent;
    }
    return size;
}

ssize_t read_bytes (int socket, char *buffer, size_t size)
{
    ssize_t bytes_received = 0;
    ssize_t bytes_remaining = size;
    while (bytes_remaining > 0)
    {
        bytes_received = read(socket, buffer, bytes_remaining);
        if (bytes_received == -1)
        {
            printf("ERROR - read() error\n");
            return -1;
        }
        bytes_remaining -= bytes_received;
        buffer += bytes_received;
    }
    return size;
}
