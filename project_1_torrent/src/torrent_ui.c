// NXC Data Communications Network torrent_ui.c for BitTorrent-like P2P File Sharing System
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#include "torrent_ui.h"
#include "torrent_engine.h"
#include "torrent_utils.h"

//// UI ELEMENTS ////  

int print_info = 0;

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
        ERROR_PRTF ("ERROR init_torrent_engine(): calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
        return NULL;
    }
    engine->port = port;
    engine->listen_sock = -1;
    size_t unique_val = getpid() + get_elapsed_msec();
    engine->engine_hash = get_hash (&unique_val, sizeof (size_t));

    engine->num_torrents = 0;
    engine->max_num_torrents = DEFAULT_ARR_MAX_NUM;
    engine->torrents = calloc (engine->max_num_torrents, sizeof (torrent_t*));
    if (engine->torrents == NULL)
    {
        ERROR_PRTF ("ERROR init_torrent_engine(): torrents calloc failed. (ERRNO %d:%s)\n", errno, strerror(errno));
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
    if (engine->listen_sock != -1)
        close (engine->listen_sock);
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

ssize_t add_torrent (torrent_engine_t *engine, HASH_t torrent_hash)
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
    torrent_idx = engine->num_torrents++;
    engine->torrents[torrent_idx] = torrent;
    update_if_max_torrent_reached (engine);
    return torrent_idx;
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

ssize_t add_peer (torrent_engine_t *engine, HASH_t torrent_hash, char *ip, int port)
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
        printf ("\t\tNAME: %s, ", torrent->torrent_name? torrent->torrent_name : "NULL");
        printf ("STATUS: ");
        if (is_torrent_info_set (torrent) == 0)
            RED_PRTF ("NO INFO\n")
        else if (get_num_completed_blocks (torrent) == torrent->num_blocks)
            GREEN_PRTF ("COMPLETED\n")
        else
            YELLOW_PRTF ("DOWNLOADING\n")
        printf ("\t\tHASH: 0x%08x, SIZE: ", torrent->torrent_hash);
        if (torrent->file_size > 1024*1024)
            printf ("%.2f MiB", (float)torrent->file_size / (1024*1024));
        else if (torrent->file_size > 1024)
            printf ("%.2f KiB", (float)torrent->file_size / 1024);
        else
            printf ("%ld B", torrent->file_size);
        printf (", BLOCKS: %ld/%ld, NUM PEERS: %ld, SPEED: %.2f KiB/s\n",
            get_num_completed_blocks(torrent), torrent->num_blocks, torrent->num_peers, 
                (double)get_torrent_download_speed(torrent)/1024.0);
        if (torrent->num_peers != 0)
        {
            printf ("\t\t\tPEERS:");
            for (size_t j = 0; j < torrent->num_peers; j++)
            {
                printf ("%s:%d (%ld/%ld) ", torrent->peers[j]->ip, torrent->peers[j]->port,
                    get_peer_num_completed_blocks (torrent->peers[j]), torrent->num_blocks);
            }
            printf ("\n");
        }
    }
}

void print_torrent_status (torrent_t *torrent)
{
    if (torrent == NULL)
        return;
    printf ("\tNAME: %s, ", torrent->torrent_name? torrent->torrent_name : "NULL");
    printf ("STATUS: ");
    if (is_torrent_info_set (torrent) == 0)
        RED_PRTF ("NO INFO\n")
    else if (get_num_completed_blocks (torrent) == torrent->num_blocks)
        GREEN_PRTF ("COMPLETED\n")
    else
        YELLOW_PRTF ("DOWNLOADING\n")
    printf ("\t\tHASH: 0x%08x, SIZE: ", torrent->torrent_hash);
        if (torrent->file_size > 1024*1024)
            printf ("%.2f MiB", (float)torrent->file_size / (1024*1024));
        else if (torrent->file_size > 1024)
            printf ("%.2f KiB", (float)torrent->file_size / 1024);
        else
            printf ("%ld B", torrent->file_size);
    printf (", BLOCKS: %ld/%ld, NUM PEERS: %ld, SPEED: %.2f KiB/s\n",
        get_num_completed_blocks(torrent), torrent->num_blocks, torrent->num_peers, 
            (double)get_torrent_download_speed(torrent)/1024.0);
    if (is_torrent_info_set (torrent) == 1)
    {
        printf ("\t\tBLOCK STATUS:");
        int leading_space_num = get_int_str_len (torrent->num_blocks);
        int skip_flag = 0;
        for (size_t i = 0; i < torrent->num_blocks; i++)
        {
            size_t row_idx = i / PRINT_COL_NUM;
            size_t row_num = torrent->num_blocks / PRINT_COL_NUM;
            if (row_idx < PRINT_SKIP_NUM || (row_num - PRINT_SKIP_NUM) < row_idx)
            {
            if (i % PRINT_COL_NUM == 0)
                YELLOW_PRTF ("\n\t\t\t%*ld ", leading_space_num, i);
            if (get_block_status (torrent, i) == B_MISSING)
                RED_PRTF ("%*s ", leading_space_num, "X")
            else if (get_block_status (torrent, i) == B_REQUESTED)
                YELLOW_PRTF ("%*s ", leading_space_num, "R")
            else if (get_block_status (torrent, i) == B_DOWNLOADED)
                GREEN_PRTF ("%*s ", leading_space_num, "O")
            else
                RED_PRTF ("%*s ", leading_space_num, "?")
            }
            else
                if (skip_flag == 0)
                {
                    printf ("\n\t\t\t%*s ", (leading_space_num + 1)*(PRINT_COL_NUM/2 + 1), "...");
                    skip_flag = 1;
                }
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
    ssize_t idx = find_torrent (engine, torrent_hash);
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
    printf ("\t\t\tIP: %s, PORT: %d\n", 
        peer->ip, peer->port);
    if (is_torrent_info_set (peer->torrent) == 1)
    {
        printf ("\t\t\tPEER BLOCK STATUS:");
        int leading_space_num = get_int_str_len (peer->torrent->num_blocks);
        int skip_flag = 0;
        for (size_t i = 0; i < peer->torrent->num_blocks; i++)
        {
            size_t row_idx = i / PRINT_COL_NUM;
            size_t row_num = peer->torrent->num_blocks / PRINT_COL_NUM;
            if (row_idx < PRINT_SKIP_NUM || (row_num - PRINT_SKIP_NUM) < row_idx)
            {
            if (i % PRINT_COL_NUM == 0)
                YELLOW_PRTF ("\n\t\t\t%*ld ", leading_space_num, i);
            if (get_peer_block_status (peer, i) == B_MISSING)
                RED_PRTF ("%*s ", leading_space_num, "X")
            else if (get_peer_block_status (peer, i) == B_REQUESTED)
                YELLOW_PRTF ("%*s ", leading_space_num, "R")
            else if (get_peer_block_status (peer, i) == B_DOWNLOADED)
                GREEN_PRTF ("%*s ", leading_space_num, "O")
            else
                RED_PRTF ("%*s ", leading_space_num, "?")
            }
            else
                if (skip_flag == 0)
                {
                    printf ("\n\t\t\t%*s ", (leading_space_num + 1)*(PRINT_COL_NUM/2 + 1), "...");
                    skip_flag = 1;
                }
        }
        printf ("\n");
    }
}

int main (int argc, char **argv)
{
    // Parse inputs
    if (argc != 2) 
    {
        printf ("Usage: %s <port>\n", argv[0]);
        printf ("ex) %s 62123\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        printf ("Invalid port number: %s\n", argv[1]);
        return 1;
    }
    printf ("Initializing torrent engine...\n");
    torrent_engine_t *torrent_engine = init_torrent_engine(port);

    while (1)
    {
        char cmd[STR_LEN] = {0};
        YELLOW_PRTF("ENTER COMMAND");
        printf(" (\"help\" for help): ");
        fflush(stdout);
        if (fgets (cmd, STR_LEN, stdin) == NULL)
            break;

        cmd[strcspn(cmd, "\n")] = '\0'; // Remove trailing newline character

        if (strncmp (cmd, "help", 5) == 0)
        {
            GREEN_PRTF ("COMMANDS:\n");
            printf ("\thelp:\n\t\tShow this help message.\n");
            printf ("\tstatus:\n\t\tShow the engine status.\n");
            printf ("\twatch:\n\t\tWatch the torrent engine status. Updates every %dms.\n", WATCH_UPDATE_MSEC);
            printf ("\tclear:\n\t\tClear the screen.\n");
            printf ("\ti:\n\t\tToggle debug info printing.\n");
            printf ("\tquit:\n\t\tQuit the program.\n");
            printf ("\twait [TIME]:\n\t\tWait for [TIME] milliseconds.\n");
            printf ("\tcreate [name] [PATH]:\n\t\tCreate torrent with [NAME] from file at [PATH].\n");
            printf ("\t\tex) create my_music music.mp3\n");
            printf ("\tadd [HASH]:\n\t\tAdd torrent with [HASH].\n");
            printf ("\t\tex) add 0x12345678\n");
            printf ("\tremove [IDX]:\n\t\tRemove torrent of index [IDX].\n");
            printf ("\t\tex) remove 0x12345678\n");
            printf ("\t[IDX]:\n\t\tShow information of torrent of index [IDX].\n");
            printf ("\t\tex) info 0x12345678\n");
            printf ("\twatch [IDX]:\n\t\tWatch the information of torrent of index [IDX]. Updates every %dms.\n", WATCH_UPDATE_MSEC);
            printf ("\t\tex) watch 0x12345678\n");
            printf ("\tadd_peer [IDX] [IP] [PORT]:\n\t\tAdd peer with [IP] and [PORT] to torrent of index [IDX].\n");
            printf ("\t\tex) add_peer 0x12345678 123:123:123:123 12345\n");
            printf ("\tremove_peer [IDX] [IP] [PORT]:\n\t\tRemove peer with [IP] and [PORT] from torrent of index [IDX].\n");
            printf ("\t\tex) remove_peer 0x12345678 123:123:123:123 12345\n\n");
        }
        else if (strncmp (cmd, "status", 7) == 0 && strlen(cmd) == 6)
        {
            GREEN_PRTF ("PRINTING ENGINE STATUS:\n");
            pthread_mutex_lock (&(torrent_engine->mutex));
            print_engine_status (torrent_engine);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            printf ("\n");
        }
        else if (strncmp (cmd, "watch", 5) == 0 && strlen(cmd) < 6)
        {
            size_t last_time = get_elapsed_msec();
            int dots = 0;
            while (kbhit() == 0)
            {
                if (get_elapsed_msec() > last_time + WATCH_UPDATE_MSEC)
                {
                    last_time = get_elapsed_msec();
                    UPDATE();
                    GOTO_X_Y (0, 0);
                    GREEN_PRTF ("WATCHING TORRENT");
                    printf (" ENGINE");
                    for (int i = 0; i < dots/2; i++)
                        printf (".");
                    for (int i = 0; i < 4 - dots/2; i++)
                        printf (" ");
                    dots = (dots + 1) % 8;
                    printf (" (Press ENTER to stop.)\n");
                    print_engine_status (torrent_engine);
                }
            }
            while (getchar() != '\n');
        }
        else if (strncmp (cmd, "clear", 6) == 0 && strlen(cmd) == 5)
        {
            UPDATE();
        }
        else if (strncmp (cmd, "i", 1) == 0 && strlen(cmd) == 1)
        {
            print_info = !print_info;
            if (print_info)
                GREEN_PRTF ("INFO PRINTING TOGGLED ON.\n\n")
            else
                GREEN_PRTF ("INFO PRINTING TOGGLED OFF.\n\n")
        }
        else if (strncmp(cmd, "quit", 5) == 0)
            break;
        else if (strncmp (cmd, "wait ", 5) == 0)
        {
            int time = 0;
            if (sscanf (cmd+5, "%d", &time) != 1 || time < 0)
            {
                RED_PRTF ("WAIT: INVALID TIME.\n\n");
                continue;
            }
            GREEN_PRTF ("WAITING %d MILLISECONDS...\n\n", time);
            usleep (time * 1000);
        }
        else if (strncmp (cmd, "add ", 4) == 0 && strlen(cmd) == 14)
        {
            HASH_t hash = str_to_hash (cmd+4);
            if (hash == 0)
            {
                RED_PRTF ("ADD: INVALID HASH.\n\n");
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            ssize_t torrent_idx = add_torrent (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (torrent_idx > 0)
            {
                GREEN_PRTF ("TORRENT 0x%08x ADDED.\n\n", hash);
            }
            else
                printf ("\n");
        }
        else if (strncmp (cmd, "create ", 7) == 0)
        {
            char path[STR_LEN] = {0};
            char name[STR_LEN] = {0};
            if (sscanf (cmd+7, "%s %s", name, path) != 2)
            {
                RED_PRTF ("CREATE: INVALID COMMAND.\n\n");
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            HASH_t hash = create_new_torrent (torrent_engine, name, path);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (hash == 0)
                printf ("\n");
            else
            {
                GREEN_PRTF ("TORRENT 0x%08x (%s) CREATED.\n\n", hash, name);
            }
        }
        else if (strncmp (cmd, "remove ", 7) == 0)
        {
            ssize_t idx = strtoll (cmd+7, NULL, 0);
            pthread_mutex_lock (&(torrent_engine->mutex));
            if (idx < 0 || idx >= torrent_engine->num_torrents)
            {
                RED_PRTF ("REMOVE: INVALID INDEX.\n\n");
                pthread_mutex_unlock (&(torrent_engine->mutex));
                continue;
            }
            HASH_t hash = torrent_engine->torrents[idx]->torrent_hash;
            int val = remove_torrent (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (val == 0)
            {
                GREEN_PRTF ("TORRENT 0x%08x REMOVED.\n\n", hash);
            }
            else
                printf ("\n");
        }
        else if (strncmp (cmd, "info ", 5) == 0)
        {
            ssize_t idx = strtoll (cmd+5, NULL, 0);
            pthread_mutex_lock (&(torrent_engine->mutex));
            if (idx < 0 || idx >= torrent_engine->num_torrents)
            {
                RED_PRTF ("INFO: INVALID INDEX.\n\n");
                pthread_mutex_unlock (&(torrent_engine->mutex));
                continue;
            }
            GREEN_PRTF ("PRINTING TORRENT STATUS:\n");
            HASH_t hash = torrent_engine->torrents[idx]->torrent_hash;
            print_torrent_status_hash (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
        }
        else if (strncmp (cmd, "watch ", 6) == 0)
        {
            ssize_t idx = strtoll (cmd+6, NULL, 0);
            pthread_mutex_lock (&(torrent_engine->mutex));
            if (idx < 0 || idx >= torrent_engine->num_torrents)
            {
                RED_PRTF ("WATCH: INVALID INDEX.\n\n");
                pthread_mutex_unlock (&(torrent_engine->mutex));
                continue;
            }
            HASH_t hash = torrent_engine->torrents[idx]->torrent_hash;
            pthread_mutex_unlock (&(torrent_engine->mutex));
            UPDATE();
            size_t last_time = get_elapsed_msec();
            int dots = 0;
            while (kbhit() == 0)
            {
                if (get_elapsed_msec() > last_time + WATCH_UPDATE_MSEC)
                {
                    last_time = get_elapsed_msec();
                    UPDATE();
                    GOTO_X_Y (0, 0);
                    GREEN_PRTF ("WATCHING TORRENT");
                    printf (" 0x%08x", hash);
                    for (int i = 0; i < dots/2; i++)
                        printf (".");
                    for (int i = 0; i < 4 - dots/2; i++)
                        printf (" ");
                    dots = (dots + 1) % 8;
                    printf (" (Press ENTER to stop.)\n");
                    print_torrent_status_hash (torrent_engine, hash);
                }
            }
            while (getchar() != '\n');
        }
        else if (strncmp (cmd, "add_peer ", 9) == 0)
        {
            printf ("COMMAND: %s\n", cmd);
            char *val = strtok (cmd, " ");
            val = strtok (NULL, " ");
            ssize_t idx = strtoll (val, NULL, 0);
            pthread_mutex_lock (&(torrent_engine->mutex));
            if (idx < 0 || idx >= torrent_engine->num_torrents)
            {
                RED_PRTF ("ADD_PEER: INVALID INDEX.\n\n");
                pthread_mutex_unlock (&(torrent_engine->mutex));
                continue;
            }
            HASH_t hash = torrent_engine->torrents[idx]->torrent_hash;
            pthread_mutex_unlock (&(torrent_engine->mutex));

            val = strtok (NULL, " ");
            int ret = check_ipv4 (val);
            if (ret == -1)
            {
                RED_PRTF ("ADD_PEER: INVALID IP - %s.\n\n", val);
                continue;
            }
            char *ip = val;

            val = strtok (NULL, " ");
            int port = atoi (val);
            if (port < 0 || port > 65535)
            {
                RED_PRTF ("ADD_PEER: INVALID PORT.\n\n");
                continue;
            }

            pthread_mutex_lock (&(torrent_engine->mutex));
            ssize_t peer_idx = add_peer (torrent_engine, hash, ip, port);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (peer_idx >= 0)
            {
                GREEN_PRTF ("PEER %s:%d ADDED to 0x%08x.\n\n", ip, port, hash);
            }
            else
                printf ("\n");
        }
        else if (strncmp (cmd, "remove_peer ", 12) == 0)
        {
            char *val = strtok (cmd, " ");
            val = strtok (NULL, " ");
            ssize_t idx = strtoll (val, NULL, 0);
            pthread_mutex_lock (&(torrent_engine->mutex));
            if (idx < 0 || idx >= torrent_engine->num_torrents)
            {
                RED_PRTF ("REMOVE_PEER: INVALID INDEX.\n\n");
                pthread_mutex_unlock (&(torrent_engine->mutex));
                continue;
            }
            HASH_t hash = torrent_engine->torrents[idx]->torrent_hash;
            pthread_mutex_unlock (&(torrent_engine->mutex));

            val = strtok (NULL, " ");
            int ret = check_ipv4 (val);
            if (ret == -1)
            {
                RED_PRTF ("REMOVE_PEER: INVALID IP - %s.\n\n", val);
                continue;
            }
            char *ip = val;

            val = strtok (NULL, " ");
            int port = atoi (val);
            if (port < 0 || port > 65535)
            {
                RED_PRTF ("REMOVE_PEER: INVALID PORT.\n\n");
                continue;
            }

            pthread_mutex_lock (&(torrent_engine->mutex));
            idx = find_peer (torrent_engine->torrents[idx], ip, port);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (idx == -1)
            {
                RED_PRTF ("REMOVE_PEER: PEER %s:%d NOT FOUND.\n\n", ip, port);
                continue;
            }
            
            pthread_mutex_lock (&(torrent_engine->mutex));
            ret = remove_peer (torrent_engine, hash, ip, port);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (ret == 0)
            {
                GREEN_PRTF ("PEER %s:%d REMOVED from 0x%08x.\n\n", ip, port, hash);
            }
            else
                printf ("\n");
        }
        else
            printf ("INVALID COMMAND. Type \"help\" for help.\n\n");
    }
    printf ("Exiting. Press ENTER.\n");
    destroy_torrent_engine(torrent_engine);
    return 0;
}