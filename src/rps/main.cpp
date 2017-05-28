#include <../panic.h>
#include <../swap.h>
#include <bwio.h>
#include <def.h>
#include <itc.h>
#include <ns.h>
#include <std.h>
#include <task.h>

using namespace ctl;

// PRNG: https://en.wikipedia.org/wiki/Xorshift
typedef unsigned PrngState;
unsigned prng(PrngState &state)
{
    unsigned x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

// Not thread-safe, but that just adds to the randomness ;)
unsigned rand() {
    static PrngState state = 1000007;
    return prng(state);
}

enum class MsgType {
    Signup = 0,
    Play,
    Quit,
};

enum class Choice {
    Paper = 0,
    Scissors,
    Rock,
    Quit,
};

struct alignas(4) Message {
    MsgType type;
    Choice choice;
};

enum class Standing {
     Won = 0,
     Lost,
     Tie,
     Quit,
     OpponentQuit,
};

void rpsServerMain() {
    ctl::registerAs(ctl::Names::RpsServer);

    // Queue is a circular buffer.
    Tid queue[NUM_RPS_CLIENTS] = {ctl::INVALID_TID, ctl::INVALID_TID};
    unsigned begin = 0, end = 0, size = 0;

    // Players in the play state.
    size_t numPlayers = 0;
    Tid players[2];

    // Round number
    int roundNum = 1;

    for (;;) {
        size_t numChoices = 0;
        Choice choices[2]{};

        // Get at least two players.
        while (size + numPlayers < 2) {
            Tid tid;
            Message msg;
            receive(&tid, msg);
            switch (msg.type) {
                case MsgType::Signup: {
                    queue[end] = tid;
                    end = (end + 1) % NUM_RPS_CLIENTS;
                    size++;
                    break;
                }

                case MsgType::Play: {
                    if (tid == players[0]) {
                        choices[0] = msg.choice;
                    } else if (tid == players[1]) {
                        choices[1] = msg.choice;
                    } else {
                        PANIC("wrong tid played");
                    }
                    numChoices++;
                    break;
                }

                case MsgType::Quit: {
                    if (tid == players[0]) {
                        // Make sure the quitted player is last.
                        swap(players[0], players[1]);
                        swap(choices[0], choices[1]);
                    }
                    choices[1] = Choice::Quit;
                    numChoices++;
                    numPlayers--;
                    break;
                }

                default: {
                    PANIC("Invalid message type");
                }
            }
        }

        // Take at most two players off of the queue and wakeup.
        while (numPlayers != 2) {
            reply(queue[begin], EmptyMessage);
            players[numPlayers] = queue[begin];
            begin = (begin + 1) % NUM_RPS_CLIENTS;
            size--;
            numPlayers++;
        }

        // Receive choices.
        while (numChoices != 2) {
            Tid tid;
            Message msg;
            receive(&tid, msg);
            switch (msg.type) {
                case MsgType::Signup: {
                    queue[end] = tid;
                    end = (end + 1) % NUM_RPS_CLIENTS;
                    size++;
                    break;
                }

                case MsgType::Play: {
                    if (tid == players[0]) {
                        choices[0] = msg.choice;
                    } else if (tid == players[1]) {
                        choices[1] = msg.choice;
                    } else {
                        PANIC("wrong tid played");
                    }
                    numChoices++;
                    break;
                }

                case MsgType::Quit: {
                    if (tid == players[0]) {
                        // Make sure the quitted player is last.
                        swap(players[0], players[1]);
                        swap(choices[0], choices[1]);
                    }
                    choices[1] = Choice::Quit;
                    numChoices++;
                    numPlayers--;
                    break;
                }

                default: {
                    PANIC("Invalid message type");
                }
            }
        }

        // Determine who won.
        Standing standings[2];
        if (choices[0] == choices[1]) {
            if (choices[0] == Choice::Quit) {
                standings[0] = standings[1] = Standing::Quit;
            } else {
                standings[0] = standings[1] = Standing::Tie;
            }
        } else if (choices[1] == Choice::Quit) {
            standings[0] = Standing::OpponentQuit;
            standings[1] = Standing::Quit;
        } else if (static_cast<Choice>((static_cast<size_t>(choices[0]) + 1) % 3) == choices[1]) {
            standings[0] = Standing::Lost;
            standings[1] = Standing::Won;
        } else {
            standings[1] = Standing::Lost;
            standings[0] = Standing::Won;
        }

        // Print into.
        bwprintf(COM2, "\r\nROCK PAPER SCISSORS ROUND %d\r\n", roundNum++);
        bwprintf(COM2, "   Tid %d played %c\r\n", players[0], "PSRQ"[static_cast<size_t>(choices[0])]);
        bwprintf(COM2, "   Tid %d played %c\r\n", players[1], "PSRQ"[static_cast<size_t>(choices[1])]);
        bwprintf(COM2, "   Tid %d standing %c\r\n", players[0], "WLTQO"[static_cast<size_t>(standings[0])]);
        bwprintf(COM2, "   Tid %d standing %c\r\n", players[1], "WLTQO"[static_cast<size_t>(standings[1])]);
        bwputstr(COM2, "Press any key to continue...\r\n");
        bwgetc(COM2);

        // Send out standings.
        for (int i = 0; i < 2; i++) {
            if (choices[i] == Choice::Quit) {
                reply(players[i], EmptyMessage);
            } else {
                reply(players[i], standings[i]);
            }
        }
    }
}

void rpsClientMain() {
    // 1. Find the RPS server by querying the name server.
    auto server = ctl::Tid(ctl::whoIs(ctl::Names::RpsServer));

    // 2. Perform a set of requests that adequately test the RPS server.
    Message msg{MsgType::Signup};
    ctl::send(server, msg, EmptyMessage);
    const unsigned n = rand() % 5;
    for (unsigned i = 0; i < n; i++) {
        msg = {MsgType::Play, static_cast<Choice>(rand() % 3)};
        Standing rply;
        ctl::send(server, msg, rply);
        bwprintf(COM2, "Tid %d received standing %c\r\n", myTid(), "WLTQO"[static_cast<int>(rply)]);
    }

    // 3. Send a quit request.
    msg = Message{MsgType::Quit};
    ctl::send(server, msg, EmptyMessage);

    // 4. Exit gracefully.
}
