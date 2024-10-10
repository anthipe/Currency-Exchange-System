#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
// Currency exchange rates
#define USD_TO_EUR 0.85
#define EUR_TO_USD 1.18
#define USD_TO_GBP 0.75
#define GBP_TO_USD 1.33
#define EUR_TO_GBP 0.88
#define GBP_TO_EUR 1.14
#define PORT 8080
#define BUFFER_SIZE 1024
void save_accounts_to_file();

// Structure for accounts
typedef struct {
    char username[50];
    double usd_balance;
    double eur_balance;
    double gbp_balance;
} Account;

// List of accounts
Account accounts[10]; // Using a static array for simplicity
int account_count = 0; // Number of accounts


// Find an account by username
Account* find_account(const char* username) {
    for (int i = 0; i < account_count; ++i) {
        if (strcmp(accounts[i].username, username) == 0) {
            return &accounts[i];
        }
    }
    return NULL;
}

// Create a new account
void create_account(const char* username) {
    if (account_count < 10) {
        strcpy(accounts[account_count].username, username);
        accounts[account_count].usd_balance = 0;
        accounts[account_count].eur_balance = 0;
        accounts[account_count].gbp_balance = 0;
        account_count++;
        save_accounts_to_file();
    }
}

// Save accounts to a file
void save_accounts_to_file() {
    FILE *file = fopen("accounts.dat", "wb");
    if (file) {
        fwrite(&accounts, sizeof(Account), account_count, file);
        fclose(file);
    } else {
        perror("Failed to open accounts.dat for writing");
    }
}

// Load accounts from a file
void load_accounts_from_file() {
    FILE *file = fopen("accounts.dat", "rb");
    if (file) {
        account_count = fread(&accounts, sizeof(Account), 10, file);
        fclose(file);
    } else {
        perror("Failed to open accounts.dat for reading");
    }
}

// Perform currency exchange
void exchange_currency(Account* acc, const char* from_currency, const char* to_currency, double amount) {
    double converted_amount = 0;

    if (strcmp(from_currency, "USD") == 0 && strcmp(to_currency, "EUR") == 0) {
        converted_amount = amount * USD_TO_EUR;
        acc->usd_balance -= amount;
        acc->eur_balance += converted_amount;
    } else if (strcmp(from_currency, "EUR") == 0 && strcmp(to_currency, "USD") == 0) {
        converted_amount = amount * EUR_TO_USD;
        acc->eur_balance -= amount;
        acc->usd_balance += converted_amount;
    } else if (strcmp(from_currency, "USD") == 0 && strcmp(to_currency, "GBP") == 0) {
        converted_amount = amount * USD_TO_GBP;
        acc->usd_balance -= amount;
        acc->gbp_balance += converted_amount;
    } else if (strcmp(from_currency, "GBP") == 0 && strcmp(to_currency, "USD") == 0) {
        converted_amount = amount * GBP_TO_USD;
        acc->gbp_balance -= amount;
        acc->usd_balance += converted_amount;
    } else if (strcmp(from_currency, "EUR") == 0 && strcmp(to_currency, "GBP") == 0) {
        converted_amount = amount * EUR_TO_GBP;
        acc->eur_balance -= amount;
        acc->gbp_balance += converted_amount;
    } else if (strcmp(from_currency, "GBP") == 0 && strcmp(to_currency, "EUR") == 0) {
        converted_amount = amount * GBP_TO_EUR;
        acc->gbp_balance -= amount;
        acc->eur_balance += converted_amount;
    } else {
        printf("Invalid currency exchange\n");
    }
}

// Handle client requests
void *client_handler(void *client_socket) {
    int sock = *(int*)client_socket;
    char buffer[BUFFER_SIZE];
    ssize_t read_size;

    printf("Client connected\n");

    // Process client commands
    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0'; // Null-terminate the string
        printf("Received from client: %s\n", buffer);

        char command[50], username[50], currency1[4], currency2[4];
        double amount;
        sscanf(buffer, "%s %s %s %lf", command, username, currency1, &amount);

        Account* acc = find_account(username);
        if (strcmp(command, "create") == 0) {
            if (!find_account(username)) {
                create_account(username);
                send(sock, "Account created successfully\n", 30, 0);
            } else {
                send(sock, "Account already exists\n", 24, 0);
            }
        } else if (strcmp(command, "balance") == 0) {
            if (acc) {
                char response[BUFFER_SIZE];
                snprintf(response, sizeof(response), "Balance for %s: USD %.2f, EUR %.2f, GBP %.2f\n",
                         acc->username, acc->usd_balance, acc->eur_balance, acc->gbp_balance);
                send(sock, response, strlen(response), 0);
            } else {
                send(sock, "Account not found\n", 19, 0);
            }
        } else if (strcmp(command, "deposit") == 0) {
            char currency[4];
            sscanf(buffer, "%s %s %s %lf", command, username, currency, &amount);
            if (acc) {
                if (strcmp(currency, "USD") == 0) {
                    acc->usd_balance += amount;
                } else if (strcmp(currency, "EUR") == 0) {
                    acc->eur_balance += amount;
                } else if (strcmp(currency, "GBP") == 0) {
                    acc->gbp_balance += amount;
                } else {
                    send(sock, "Invalid currency\n", 18, 0);
                    continue;
                }
                save_accounts_to_file();
                send(sock, "Deposit successful\n", 20, 0);
            } else {
                send(sock, "Account not found\n", 19, 0);
            }
        } else if (strcmp(command, "withdraw") == 0) {
            char currency[4];
            sscanf(buffer, "%s %s %s %lf", command, username, currency, &amount);
            if (acc) {
                if (strcmp(currency, "USD") == 0 && acc->usd_balance >= amount) {
                    acc->usd_balance -= amount;
                } else if (strcmp(currency, "EUR") == 0 && acc->eur_balance >= amount) {
                    acc->eur_balance -= amount;
                } else if (strcmp(currency, "GBP") == 0 && acc->gbp_balance >= amount) {
                    acc->gbp_balance -= amount;
                } else {
                    send(sock, "Insufficient funds or invalid currency\n", 41, 0);
                    continue;
                }
                save_accounts_to_file();
                send(sock, "Withdrawal successful\n", 23, 0);
            } else {
                send(sock, "Account not found\n", 19, 0);
            }
        } else if (strcmp(command, "exchange") == 0) {
            char from_currency[4], to_currency[4];
            sscanf(buffer, "%s %s %s %s %lf", command, username, from_currency, to_currency, &amount);
            if (acc) {
                if (amount <= 0) {
                    send(sock, "Invalid amount\n", 15, 0);
                } else if (strcmp(from_currency, to_currency) == 0) {
                    send(sock, "Cannot exchange the same currency\n", 34, 0);
                } else {
                    exchange_currency(acc, from_currency, to_currency, amount);
                    save_accounts_to_file();
                    send(sock, "Exchange successful\n", 21, 0);
                }
            } else {
                send(sock, "Account not found\n", 19, 0);
            }
        } else {
            send(sock, "Invalid command\n", 16, 0);
        }
    }

    if (read_size == 0) {
        printf("Client disconnected\n");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(sock);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t thread_id;

    // Load accounts from file
    load_accounts_from_file();

    // Create the socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening on the socket
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // Create a new thread for the client
        if (pthread_create(&thread_id, NULL, client_handler, (void*)&new_socket) < 0) {
            perror("could not create thread");
            close(new_socket);
        }
    }

    return 0;
}
