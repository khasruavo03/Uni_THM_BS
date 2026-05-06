#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    int val;
    struct Node *next;
} Node;

// Neue Zahl
void addNum(Node **head, int value) {
    struct Node *newNode = malloc(sizeof(struct Node));
    newNode -> val = value;
    newNode -> next = NULL;

    if (*head == NULL) {
        *head = newNode;
        return;
    }

    Node *current = *head;
    while (current -> next != NULL) {
        current -> next = newNode;
    }

    current -> next = newNode;
}

// Sortieren der Liste nach Bubblesort
void sortList(Node *head) {
    if (head == NULL) {
        return;
    }

    int swapped;
    struct Node *ptr;
    struct Node *last = NULL;

    do {
        swapped = 0;
        ptr = head;
        while(ptr->next != last) {
            if(ptr->val > ptr->next->val) {
                int temp = ptr -> val;
                ptr->val = ptr->next->val;
                ptr->next->val = temp;
                swapped = 1;
            }
            ptr = ptr->next;
        }
        last = ptr;
    } while (swapped);
}

void printList(Node *head) {
    Node *temp = head;
    
    while(temp != NULL) {
        printf(" %d", temp->val);
        temp = temp->next;
    }

    printf("\n");
}

int exists(Node *head, int value) {
    Node *current = head;

    while (current != NULL) {
        if (current->val == value) {
            return 1;
        }
        current = current->next;
    }
    
    return 0;
}

void cleanup_memory(Node *head) {
    Node *current = head;
    Node *tmp;

    while (current != NULL) {
        tmp = current;
        current = current->next;
        free(tmp);
    }

    printf("\n==================\n");
    printf("Cleaning up memory\n");
    printf("==================\n");

    exit(0);
}

int main(int argc, char *argv[]) {
    Node *head = NULL;
    char buffer[1024];

    while (1) {
        printf("Eingabe: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;

        // Entferne Zeilenumbruch am Ende
        buffer[strcspn(buffer, "\n")] = 0;

        // Tokenisierung der Eingabe (Zahlen getrennt durch Leerzeichen)
        char *token = strtok(buffer, " ");
        while (token != NULL) {
            char *end;
            long val = strtol(token, &end, 10);

            if (*end == '\0') {
                if (!exists(head, val)) {
                    addNum(&head, val);
                }
            }
            token = strtok(NULL, " ");
        }

        // Sortieren und Ausgeben
        sortList(head);
        printList(head);
    }

    // freeList(head);
    return 0;
}