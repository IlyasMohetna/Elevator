#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Function to create a message queue
int create_message_queue() {
    int file_id;

    if ((file_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666)) == -1) {
        perror("[IPC Utils] Error creating message queue");
        exit(1); // Terminate the program on failure
    }

    printf("[IPC Utils] Message queue ID: %d\n", file_id);
    return file_id; // Return the message queue ID
}
