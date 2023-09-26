// NXC Data Communications torrent_utils.c for torrent
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 19


///// DO NOT MODIFY THIS FILE!! ////


#include "torrent_utils.h"


//// UTILITY FUNCTIONS ////

HASH_t get_hash (void* data, size_t len)
{
    if (data == NULL || len == 0)
    {
        ERROR_PRTF ("ERROR get_hash(): invalid arguments.\n");
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

HASH_t str_to_hash (char *str)
{
    if (str == NULL)
    {
        ERROR_PRTF ("ERROR str_to_hash(): invalid arguments.\n");
        return 0;
    }
    if (strncmp (str, "0x", 2) != 0)
        return 0;
    HASH_t hash = 0;
    for (size_t i = 2; i < 10; i++)
    {
        hash <<= 4;
        if (str[i] >= '0' && str[i] <= '9')
            hash += str[i] - '0';
        else if (str[i] >= 'a' && str[i] <= 'f')
            hash += str[i] - 'a' + 10;
        else if (str[i] >= 'A' && str[i] <= 'F')
            hash += str[i] - 'A' + 10;
        else
            return 0;
    }
    return hash;
}

int check_ipv4 (char *ip)
{
    if (ip == NULL)
    {
        ERROR_PRTF ("ERROR check_ipv4(): invalid arguments.\n");
        return -1;
    }
    int num = -1;
    int dots = 0;
    int nums = 0;
    int i = 0;
    for (; i < 12; i++)
    {
        if (num == -1)
        {
            if (ip[i] >= '0' && ip[i] <= '9')
            {
                num = ip[i] - '0';
                nums++;
            }
            else
            {
                ERROR_PRTF ("ERROR check_ipv4() 1: num %d, nums %d, dots %d, i %d, ip %c\n", 
                    num, nums, dots, i, ip[i]);
                return -1;
            }
        }
        else
        {
            if (ip[i] >= '0' && ip[i] <= '9')
            {
                num *= 10;
                num += ip[i] - '0';
                nums++;
                if (nums > 3 || num > 255)
                {
                    ERROR_PRTF ("ERROR check_ipv4() 2: num %d, nums %d, dots %d, i %d, ip %c\n", 
                        num, nums, dots, i, ip[i]);
                    return -1;
                }
            }
            else if (ip[i] == '.')
            {
                if (num > 255 || nums == 0)
                {
                    ERROR_PRTF ("ERROR check_ipv4() 3: num %d, nums %d, dots %d, i %d, ip %c\n", 
                        num, nums, dots, i, ip[i]);
                    return -1;
                }
                num = -1;
                nums = 0;
                dots++;
                if (dots > 3)
                {
                    ERROR_PRTF ("ERROR check_ipv4() 4: num %d, nums %d, dots %d, i %d, ip %c\n", 
                        num, nums, dots, i, ip[i]);
                    return -1;
                }
            }
            else if (dots == 3 && nums > 0)
            {
                if (num > 255)
                {
                    ERROR_PRTF ("ERROR check_ipv4() 5: num %d, nums %d, dots %d, i %d, ip %c\n", 
                        num, nums, dots, i, ip[i]);
                    return -1;
                }
                break;
            }
            else 
            {
                ERROR_PRTF ("ERROR check_ipv4() 6: num %d, nums %d, dots %d, i %d, ip %c\n", 
                    num, nums, dots, i, ip[i]);
                return -1;
            }
        }
    }
    return i;
}

size_t get_time_msec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000ULL) + (tv.tv_usec/1000ULL);
}

size_t get_elapsed_msec()
{
    static size_t init_time = 0;
    if (init_time == 0)
        init_time = get_time_msec();
    return get_time_msec() - init_time;
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
        ERROR_PRTF ("ERROR read_file(): invalid arguments.\n");
        return -1;
    }

    FILE *fp = fopen (path, "rb");
    if (fp == NULL)
    {
        ERROR_PRTF ("ERROR read_file(): fopen failed.\n");
        return -1;
    }
        
    fseek (fp, 0, SEEK_END);
    size_t len = ftell (fp);
    fseek (fp, 0, SEEK_SET);

    if (fread (data, 1, len, fp) != len)
    {
        ERROR_PRTF ("ERROR read_file(): fread failed.\n");
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
        ERROR_PRTF ("ERROR read_file(): fopen failed.\n");
        return -1;
    }

    fseek (fp, 0, SEEK_END);
    size_t len = ftell (fp);
    fclose (fp);

    return len;
}

ssize_t write_bytes (int socket, void *buffer, size_t size)
{
    ssize_t bytes_sent = 0;
    ssize_t bytes_remaining = size;
    signal(SIGPIPE, SIG_IGN);
    while (bytes_remaining > 0)
    {
        bytes_sent = write(socket, buffer, bytes_remaining);
        if (bytes_sent == -1)
        {
            ERROR_PRTF ("ERROR write_bytes(): write failed.\n");
            return -1;
        }
        bytes_remaining -= bytes_sent;
        buffer += bytes_sent;
    }
    return size;
}

ssize_t read_bytes (int socket, void *buffer, size_t size)
{
    ssize_t bytes_received = 0;
    ssize_t bytes_remaining = size;
    while (bytes_remaining > 0)
    {
        bytes_received = read(socket, buffer, bytes_remaining);
        if (bytes_received == -1)
        {
            ERROR_PRTF ("ERROR read_bytes(): read failed\n");
            return -1;
        }
        bytes_remaining -= bytes_received;
        buffer += bytes_received;
    }
    return size;
}

int kbhit()
{
    // Initialize poll strucutre.
    struct pollfd fds;
    // We want to poll stdin (fd 0).
    fds.fd = STDIN_FILENO;
    // We want to poll for input events.
    fds.events = POLLIN;

    // Poll stdin to see if there is any input waiting.
    // Returns > 0 if there is, 0 if there is not, or -1 on error.
    int ret = poll(&fds, 1, TIMEOUT_MSEC);

    if (ret == -1)
    {
        ERROR_PRTF ("ERROR kbhit(): poll failed.\n");
        return 0;
    }
    
    // Check if bit for POLLIN is set.
    return (fds.revents & POLLIN);
}