/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: Bernhard Jaeger
 *
 * Created on June 1, 2018, 2:17 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curl/curl.h>

size_t write_data(char *buffer, size_t size, size_t nmemb, void *userp)
{
    /*Since we are not interested in the response just do nothing here.*/
    return size * nmemb;
}


static size_t header_callback(char *buffer, size_t size,
                              size_t nitems, void *userdata)
{
  /* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
  /* 'userdata' is set with CURLOPT_HEADERDATA */
    printf("%s", buffer);
  return nitems * size;
}

/*
 *
 */
int main(int argc, char** argv) 
{
    int c , bool3 = 0, bool4 = 0, bool6 = 0, boolTls = 0, bool2 = 0, bool_fast_open = 0;
    char *url, *ip;
    int port;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl;
    
    while ((c = getopt(argc, argv, "4632Fu:p:x")) != -1)
    {
        switch (c)
        {
        case 'x':
            boolTls = 1;
            break;
        case '4':
            bool4 = 1;
            break;
        case '6':
            bool6 = 1;
            break;
        case '3':
            bool3 = 1;
            break;
        case '2':
            bool2 = 1;
            break;
        case 'F':
            bool_fast_open = 1;
            break;
        case 'u':
            url = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            fprintf(stderr,
                    "Options:\n"
                    "-4 To force usage of IPv4\n"
                    "-6 To force usage of IPv6\n"
                    "-3 To use TLS 1.3 instead of TLS 1.2\n"
                    "-u URL The Url to connect to\n"
                    "-p port The port to connect to\n"
                    "-x second mode. Outputs only URL if connection was successful\n");
            exit(EXIT_FAILURE);
        }
    }
    curl = curl_easy_init();
    
    if(curl)
    {
        CURLcode res;
        char errbuf[CURL_ERROR_SIZE];

        /* provide a buffer to store errors in */
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

        /* set the error buffer as empty before performing a request */
        errbuf[0] = 0;

        if(bool4 == 1)
        {
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        }
        else if (bool6 == 1)
        {
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
        }
        else
        {
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER);
        }

        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url); /*test for tls 1.3 https://tls13.akamai.io*/
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        curl_easy_setopt(curl, CURLOPT_PORT, port);
        //curl_easy_setopt(curl, CURLOPT_CAPATH, "/etc/ssl/certs");/*Need to use this since the standard ca path on my system seems to be broken*/
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); /*For Debug*/
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        //curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

        if(bool_fast_open == 1)
        {
            curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);
        }
        /* HTTP/2 */
        if(bool2 == 1)
        {
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
        }

        if(bool3 == 1)
        {
            curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_3 | CURL_SSLVERSION_MAX_TLSv1_3); /*Since libcurl 7.61 CURL_SSLVERSION sets the minimum and not the exact so we have to or the max version to it to get the exact version*/
        }
        else
        {
            curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2 | CURL_SSLVERSION_MAX_TLSv1_2); /*TODO:Max version still needs testing*/
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        res = curl_easy_perform(curl);
        if(res == CURLE_OK)
        {
            if(boolTls == 1)
            {
                printf("%s\n", url);
            }
            else
            {
                double connect_dns, connect_tcp, connect_tls, ttfb, total_time;
                res = curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &connect_dns);
                if(CURLE_OK == res)
                {
                    time_t rawtime;
                    time(&rawtime);
                    printf("%li;%s;", (long)rawtime, url);
                    res = curl_easy_getinfo(curl, CURLINFO_PRIMARY_IP, &ip);
                    if(CURLE_OK == res)
                    {
                        printf("%s;%d;", ip, port);
                        res = curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME, &connect_tls);
                        if(CURLE_OK == res)
                        {
                            long code;
                            res = curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &connect_tcp);
                            printf("%.3f;%.3f;%.3f;", connect_dns * 1000.0, (connect_tcp - connect_dns) * 1000.0, (connect_tls - connect_dns) * 1000.0);

                            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
                            res = curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &ttfb);
                            printf("%.3f;", ttfb * 1000.0);
                            res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
                            printf("%.3f;", total_time * 1000.0);
                            if(CURLE_OK == res)
                            {
                                printf("%3ld;",code);
                                long http_version;
                                curl_easy_getinfo(curl, CURLINFO_HTTP_VERSION, &http_version);
                                switch(http_version)
                                {
                                    case 0:
                                    {
                                        printf("Unknown;");
                                        break;
                                    }
                                    case 1:
                                    {
                                        printf("HTTP/1;");
                                        break;
                                    }
                                    case 2:
                                    {
                                        printf("HTTP/1.1;");
                                        break;
                                    }
                                    case 3:
                                    {
                                        printf("HTTP2;");
                                        break;
                                    }

                                }
                                printf("TCP/");
                                if(bool3 == 1)
                                {
                                    printf("TLS1.3\n");
                                }
                                else
                                {
                                    printf("TLS1.2\n");
                                }
                            }
                        }
                    }
                }
                else
                {
                    fprintf(stderr, "\nUnsupported option.\n");
                }
            }
        }
        else
        {
            size_t len = strlen(errbuf);
            fprintf(stderr, "\nlibcurl: (%d) ", res);
            if(len)
              fprintf(stderr, "%s%s", errbuf,
                      ((errbuf[len - 1] != '\n') ? "\n" : ""));
            else
              fprintf(stderr, "%s\n", curl_easy_strerror(res));
            if(boolTls == 1)
            {
                char *result = malloc(strlen("www.") + strlen(url) + 1);
                strcpy(result, "www.");
                strcat(result, url);
                curl_easy_setopt(curl, CURLOPT_URL, result);
                res = curl_easy_perform(curl);
                if(res == CURLE_OK)
                {
                    printf("%s\n", result);
                }
                else
                {
                    fprintf(stderr, "Err: %i\n", (int)res);
                }
                free(result);
            }
        }
        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();
    
    return (EXIT_SUCCESS);
}

