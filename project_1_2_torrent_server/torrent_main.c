// NXC Data Communications torrent_main.c for torrent 
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////

#include "torrent_functions.h"

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
    // update();
    printf ("Initializing torrent engine...\n");
    torrent_engine_t *torrent_engine = init_torrent_engine(port);

    while (1)
    {
        char cmd[STR_LEN] = {0};
        yellow();
        printf("ENTER COMMAND");
        reset();
        printf(" (\"help\" for help): ");
        fflush(stdout);
        if (fgets (cmd, STR_LEN, stdin) == NULL)
            break;

        cmd[strcspn(cmd, "\n")] = '\0'; // Remove trailing newline character

        if (strncmp (cmd, "help", 5) == 0)
        {
            printf ("COMMANDS:\n");
            printf ("\thelp:\n\t\tShow this help message.\n");
            printf ("\tstatus:\n\t\tShow the engine status.\n");
            printf ("\tadd [HASH]:\n\t\tAdd torrent with [HASH].\n");
            printf ("\t\tex) add 0x12345678\n");
            printf ("\tremove [HASH]:\n\t\tRemove torrent with [HASH].\n");
            printf ("\t\tex) remove 0x12345678\n");
            printf ("\tcreate [name] [PATH]:\n\t\tCreate torrent with [NAME] from file at [PATH].\n");
            printf ("\t\tex) create my_music music.mp3\n");
            printf ("\tinfo [HASH]:\n\t\tShow information of torrent with [HASH].\n");
            printf ("\t\tex) info 0x12345678\n");
            printf ("\twatch [HASH]:\n\t\tWatch the information of torrent with [HASH]. Updates every 0.1ms.\n");
            printf ("\t\tex) watch 0x12345678\n");
            printf ("\tadd_peer [HASH] [IP] [PORT]:\n\t\tAdd peer with [IP] and [PORT] to torrent with [HASH].\n");
            printf ("\t\tex) add_peer 0x12345678 123:123:123:123 12345\n");
            printf ("\tremove_peer [HASH] [IP] [PORT]:\n\t\tRemove peer with [IP] and [PORT] from torrent with [HASH].\n");
            printf ("\t\tex) remove_peer 0x12345678 123:123:123:123 12345\n");
            printf ("\twait [TIME]:\n\t\tWait for [TIME] milliseconds.\n");
            printf ("\tquit:\n\t\tQuit the program.\n\n");
        }
        else if (strncmp (cmd, "status", 7) == 0 && strlen(cmd) == 6)
        {
            green();
            printf ("PRINTING ENGINE STATUS:\n");
            reset();
            pthread_mutex_lock (&(torrent_engine->mutex));
            print_engine_status (torrent_engine);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            printf ("\n");
        }
        else if (strncmp (cmd, "add ", 4) == 0 && strlen(cmd) == 14)
        {
            HASH_t hash = str_to_hash (cmd+4);
            if (hash == 0)
            {
                red();
                printf ("ADD: INVALID HASH.\n\n");
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            int val = add_torrent (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (val == 0)
            {
                green();
                printf ("TORRENT 0x%x ADDED.\n\n", hash);
                reset();
            }
            else
                printf ("\n");
        }
        else if (strncmp (cmd, "remove ", 7) == 0 && strlen(cmd) == 17)
        {
            HASH_t hash = str_to_hash (cmd+7);
            if (hash == 0)
            {
                red();
                printf ("REMOVE: INVALID HASH.\n\n");
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            size_t idx = find_torrent (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (idx == -1)
            {
                red();
                printf ("WATCH: TORRENT 0x%x NOT FOUND.\n\n", hash);
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            int val = remove_torrent (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (val == 0)
            {
                green();
                printf ("TORRENT 0x%x REMOVED.\n\n", hash);
                reset();
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
                red();
                printf ("CREATE: INVALID COMMAND.\n\n");
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            HASH_t hash = create_new_torrent (torrent_engine, name, path);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (hash == 0)
                printf ("\n");
            else
            {
                green();
                printf ("TORRENT 0x%x CREATED.\n\n", hash);
                reset();
            }
        }
        else if (strncmp (cmd, "info ", 5) == 0 && strlen(cmd) == 15)
        {
            HASH_t hash = str_to_hash (cmd+5);
            if (hash == 0)
            {
                red();
                printf ("INFO: INVALID HASH.\n\n");
                reset();
                continue;
            }
            green();
            printf ("PRINTING TORRENT STATUS:\n");
            reset();
            pthread_mutex_lock (&(torrent_engine->mutex));
            print_torrent_status_hash (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
        }
        else if (strncmp (cmd, "watch ", 6) == 0 && strlen(cmd) == 16)
        {
            HASH_t hash = str_to_hash (cmd+6);
            if (hash == 0)
            {
                red();
                printf ("WATCH: INVALID HASH.\n\n");
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            size_t idx = find_torrent (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (idx == -1)
            {
                printf ("WATCH: TORRENT 0x%x NOT FOUND.\n\n", hash);
                continue;
            }
            update();
            size_t last_time = get_time_msec();
            while (kbhit() == 0)
            {
                if (get_time_msec() > last_time + 100)
                {
                    last_time = get_time_msec();
                    gotoxy (0, 0);
                    green();
                    printf ("WATCHING TORRENT 0x%x... (Press enter to stop.)\n", hash);
                    reset();
                    pthread_mutex_lock (&(torrent_engine->mutex));
                    print_torrent_status_hash (torrent_engine, hash);
                    pthread_mutex_unlock (&(torrent_engine->mutex));
                }
            }
            while (getchar() != '\n');
        }
        else if (strncmp (cmd, "add_peer ", 9) == 0)
        {
            char ip[STR_LEN] = {0};
            int port = 0;
            HASH_t hash = str_to_hash (cmd+9);
            if (hash == 0)
            {
                red();
                printf ("ADD_PEER: INVALID HASH.\n\n");
                reset();
                continue;
            }
            int val = check_ipv4 (cmd+20);
            if (val == -1)
            {
                red();
                printf ("ADD_PEER: INVALID IP.\n\n");
                reset();
                continue;
            }
            memcpy (ip, cmd+20, val);
            if (sscanf (cmd+20+val , "%d", &port) != 1 || port < 0 || port > 65535)
            {
                red();
                printf ("ADD_PEER: INVALID PORT.\n\n");
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            val = add_peer (torrent_engine, hash, ip, port);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (val == 0)
            {
                green();
                printf ("PEER %s:%d ADDED to 0x%x.\n\n", ip, port, hash);
                reset();
            }
            else
                printf ("\n");
        }
        else if (strncmp (cmd, "remove_peer ", 12) == 0)
        {
            char ip[STR_LEN] = {0};
            int port = 0;
            HASH_t hash = str_to_hash (cmd+12);
            if (hash == 0)
            {
                red();
                printf ("REMOVE_PEER: INVALID HASH.\n\n");
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            size_t idx = find_torrent (torrent_engine, hash);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (idx == -1)
            {
                red();
                printf ("WATCH: TORRENT 0x%x NOT FOUND.\n\n", hash);
                reset();
                continue;
            }
            int val = check_ipv4 (cmd+23);
            if (val == -1)
            {
                red();
                printf ("REMOVE_PEER: INVALID IP.\n\n");
                reset();
                continue;
            }
            memcpy (ip, cmd+23, val);
            if (sscanf (cmd+23+val, "%d", &port) != 1 || port < 0 || port > 65535)
            {
                red();
                printf ("REMOVE_PEER: INVALID PORT.\n\n");
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            idx = find_peer (torrent_engine->torrents[idx], ip, port);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (idx == -1)
            {
                red();
                printf ("REMOVE_PEER: PEER %s:%d NOT FOUND.\n\n", ip, port);
                reset();
                continue;
            }
            pthread_mutex_lock (&(torrent_engine->mutex));
            val = remove_peer (torrent_engine, hash, ip, port);
            pthread_mutex_unlock (&(torrent_engine->mutex));
            if (val == 0)
            {
                green();
                printf ("PEER %s:%d REMOVED from 0x%x.\n\n", ip, port, hash);
                reset();
            }
            else
                printf ("\n");
        }
        else if (strncmp (cmd, "wait ", 5) == 0)
        {
            int time = 0;
            if (sscanf (cmd+5, "%d", &time) != 1 || time < 0)
            {
                red();
                printf ("WAIT: INVALID TIME.\n\n");
                reset();
                continue;
            }
            green();
            printf ("WAITING %d MILLISECONDS...\n\n", time);
            reset();
            usleep (time * 1000);
        }
        else if (strncmp(cmd, "quit", 5) == 0)
            break;
        else
            printf ("INVALID COMMAND. Type \"help\" for help.\n\n");
    }
    printf ("Quitting...\n");
    destroy_torrent_engine(torrent_engine);
    return 0;
}