Color COLORS[] = {OFF, RED, ORANGE, YELLOW, GREEN, BLUE, MAGENTA};
Color ROLE_COLORS[] = { OFF, BLUE, GREEN, RED, YELLOW };
enum MODE {
  EMPTY = 0,
  IDLE_MODE,
  SETUP_MODE,
  COLOR_CHOOSING_MODE,
  ACTION_PHASE_MODE,
  VOTING_PHASE_MODE,
  RESULTS_PHASE_MODE,
  LOSS_MODE,
  VICTORY_MODE,
  NUM_MODES
} currentMode = IDLE_MODE;

const byte BOARD_ID_START_VAL = NUM_MODES + 1; //Must be greater than NUM_MODES
const byte PLAYER_ID_START_VAL = BOARD_ID_START_VAL + 8; //Must be at least 7 greater than BOARD_ID_START_VAL
const byte PLAYER_COUNTER_START_VAL = PLAYER_ID_START_VAL + 8; //Must be greater than NUM_MODES
const byte NUM_PLAYERS_START_VAL = PLAYER_COUNTER_START_VAL + 8; //Must be at least 7 greater than PLAYER_COUNTER_START_VAL
const byte TURN_DONE = NUM_PLAYERS_START_VAL + 8; //Must be at least 7 greater than NUM_PLAYERS_START_VAL
bool isActionRoot = false;
bool isVotingRoot = false;
bool isResultsRoot = false;

enum ROLE_TYPE { NIL = 0, KNIGHT, WIZARD, EVIL, NORMAL };

//Data for this Blink
enum PIECE_TYPE { BOARD, PLAYER } type = BOARD;
byte id = 0; //Valid ID range: 1-6
byte idFace = 0; //Face on which the color for this Blink should be shown
byte pointerFace = 0; //Face opposite the idFace
byte nextFace;
byte prevFace;
byte numPlayers;
bool isDeaf = false;
ROLE_TYPE roles[] = {NIL, NORMAL, KNIGHT, NORMAL, WIZARD, EVIL, NORMAL};  // Stores the roles of all the blinks
bool isRoleDisplayed = false; //true if the player's role is currently being displayed on this Blink
byte currentPlayerId = 0; //The ID of the player who is currently taking a turn
bool isKilled = false;
bool isProtected = false;
byte connectedPlayerId = 0;
byte numVotes = 0;
byte propActionVal = 0;
bool roundStarted = false;
bool isTurnDone = false;
bool isMyTurn = false;
bool votingStarted = false;
bool resultsStarted = false;
bool resultsDone = false;

Timer resultsTimer;
Timer actionAnimationTimer;

#define HAVE_PLAYER_CONNECTED (!isValueReceivedOnFaceExpired(idFace))
#define IS_ALIVE(id) (roles[id] != NIL)
#define MY_COLOR (COLORS[id])
#define MY_ROLE (roles[id])
#define MY_ROLE_COLOR (ROLE_COLORS[MY_ROLE])
#define REGULATED_ID(id) ((id-1)%6+1) //For any value, return an ID from 1-6

void setup() {
  // put your setup code here, to run once:
  randomize();
}

void loop() {
  //Reset on long press
  if (buttonLongPressed()) { //NOTE This cannot be the same as the top condition in setupLoop()
    id = 0;
    setValueSentOnAllFaces(EMPTY);
    type = BOARD;
    currentMode = IDLE_MODE;
    setColor(OFF);
    idFace = 0;
    pointerFace = 0;
    nextFace = 0;
    prevFace = 0;
    numPlayers = 0;
    isDeaf = false;
    isRoleDisplayed = false;
    currentPlayerId = 0;
    isKilled = false;
    isProtected = false;
    connectedPlayerId = 0;
    numVotes = 0;
    propActionVal = 0;
    roundStarted = false;
    isTurnDone = false;
    isMyTurn = false;
    votingStarted = false;
    resultsStarted = false;
    resultsDone = false;
    isActionRoot = false;
    isVotingRoot = false;
    isResultsRoot = false;
  }

  //If already set up...
  if (id > 0) {
    checkForNewMode();

    switch (currentMode) {
      case COLOR_CHOOSING_MODE:
        colorChoosingLoop();
        break;

      case ACTION_PHASE_MODE:
        actionPhaseLoop();
        break;

      case VOTING_PHASE_MODE:
        votingPhaseLoop();
        break;

      case RESULTS_PHASE_MODE:
        resultsPhaseLoop();
        break;

      case LOSS_MODE:
        lossLoop();
        break;

      case VICTORY_MODE:
        victoryLoop();
        break;
    }
  }

  //Do setup
  else {
    setupLoop();
  }
}

void setupLoop() {
  //If button is double clicked, start off chain
  if (buttonDoubleClicked()) { //NOTE This cannot be the same as the top condition for setup in loop()
    //Look for an available face to send ID to
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        //Send ID 1
        setValueSentOnFace(BOARD_ID_START_VAL + 1, f);
        nextFace = f;
        setColorOnFace(BLUE, f);
        break;
      }
    }
  }

  //Listen for an ID
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte val = getLastValueReceivedOnFace(f);
      if (val > BOARD_ID_START_VAL && val <= BOARD_ID_START_VAL + FACE_COUNT) { //Make sure the 1-offset for the IDs is accounted for
        //ID is found, and this is a board Blink
        type = BOARD;
        id = val - BOARD_ID_START_VAL;
        prevFace = f;
        setColorOnFace(GREEN, f);

        if (id < FACE_COUNT) { //The root will always get its ID last, so it will be equal to FACE_COUNT
          //Look for an available face to send ID to
          FOREACH_FACE(f) {
            if (!isValueReceivedOnFaceExpired(f) && f != prevFace) {
              //Send next ID
              setValueSentOnFace(val + 1, f);
              nextFace = f;
              setColorOnFace(BLUE, f);
            }
          }
        }
        setColor(OFF);
        changeMode(COLOR_CHOOSING_MODE);
      } else if (val > PLAYER_ID_START_VAL && val <= PLAYER_ID_START_VAL + FACE_COUNT + 1) { //Make sure the 1-offset for the IDs is accounted for
        //ID is found, and this is a player Blink
        type = PLAYER;
        id = val - PLAYER_ID_START_VAL;
        idFace = f;
        prevFace = idFace;
        pointerFace = (idFace + 3) % FACE_COUNT;
        nextFace = pointerFace;
        changeMode(ACTION_PHASE_MODE);
        setColorOnFace(COLORS[id], idFace);
      }
    }
  }
}

/**
   Look at prevFace for mode change information
*/
MODE checkForNewMode() {
  if (!isValueReceivedOnFaceExpired(prevFace)) {
    byte value = getLastValueReceivedOnFace(prevFace);

    //Look for a mode that matches
    for (byte m = IDLE_MODE; m < NUM_MODES; m++) {
      if (value == m && currentMode != m) {
        changeMode(m);
      }
    }
  }

  //Go into action phase mode if I am a player Blink and alone
  if (type == PLAYER && isAlone() && currentMode != ACTION_PHASE_MODE) {
    changeMode(ACTION_PHASE_MODE);
  }
}

void colorChoosingLoop() {
  //Continue to send id to next Blink
  setValueSentOnFace(id + BOARD_ID_START_VAL + 1, nextFace);

  //Calculate the outward-facing face of this Blink
  idFace = (nextFace + 2) % FACE_COUNT;
  if (idFace == prevFace) {
    idFace = (prevFace + 2) % FACE_COUNT;
  }
  pointerFace = (idFace + 3) % FACE_COUNT;

  //Set this Blink's color
  setColorOnFace(COLORS[id], idFace);

  //Send ID to player Blinks
  setValueSentOnFace(id + PLAYER_ID_START_VAL, idFace);

  //Listen for single-press to indicate game start
  if (buttonSingleClicked()) {
    roundStarted = false;
    setColor(WHITE);
    isActionRoot = true;
    setValueSentOnFace(ACTION_PHASE_MODE, nextFace);
    changeMode(ACTION_PHASE_MODE);
  }
}

void actionPhaseLoop() {
  if (type == PLAYER) {
    //Broadcast my ID on my idFace
    setValueSentOnFace(EMPTY, pointerFace);
    setValueSentOnFace(id + PLAYER_ID_START_VAL, idFace);
    setColorOnFace(COLORS[id], idFace);

    //Listen for single click to toggle role color display
    if (buttonSingleClicked()) {
      isRoleDisplayed = !isRoleDisplayed;
    }

    //Toggle role color display
    if (isRoleDisplayed) {
      FOREACH_FACE(f) {
        if (f != idFace) {
          setColorOnFace(MY_ROLE_COLOR, f);
        }
      }
    } else {
      FOREACH_FACE(f) {
        if (f != idFace) {
          setColorOnFace(OFF, f);
        }
      }
    }
  } else if (type == BOARD) {
    //Reset voting phase vals
    votingStarted = false;
    isVotingRoot = false;

    //Broadcast nothing on idFace
    setValueSentOnFace(EMPTY, idFace);

    setColorOnFace(COLORS[currentPlayerId], pointerFace);
    byte prevVal = getLastValueReceivedOnFace(prevFace);

    //Propagate mode
    if (isActionRoot) {
      if (!roundStarted) {
        if (prevVal != ACTION_PHASE_MODE) {
          propActionVal = ACTION_PHASE_MODE;
        } else {
          setColor(GREEN);
          roundStarted = true;
          isMyTurn = true;
        }
      } else {
        setColor(BLUE);
      }
      if (getLastValueReceivedOnFace(prevFace) == TURN_DONE) {
        //Go to voting phase
        isVotingRoot = true;
        setValueSentOnFace(VOTING_PHASE_MODE, nextFace);
        changeMode(VOTING_PHASE_MODE);
      }
      if (isTurnDone) {
        isMyTurn = false;
      }
    } else {
      //Beginning animation
      if (!roundStarted) {
        actionAnimationTimer.set(1000);
        roundStarted = true;
      }
      if (!actionAnimationTimer.isExpired()) {
        setColorOnFace(WHITE, pointerFace);
      }

      if (!isTurnDone) {
        propActionVal = ACTION_PHASE_MODE;
      }
      isMyTurn = (getLastValueReceivedOnFace(prevFace) == TURN_DONE) && !isTurnDone;
    }

    if (isMyTurn) {
      //Skip if I am dead
      if (MY_ROLE == NIL) {
        propActionVal = TURN_DONE;
        isTurnDone = true;
      } else {
        //Set pointer color
        setColorOnFace(WHITE, pointerFace);
      }

      //Check for button press to end turn
      if (buttonSingleClicked()) {
        propActionVal = TURN_DONE;
        isTurnDone = true;
      }
    } else {
      setColorOnFace(OFF, pointerFace);
    }

    //Colors handling
    if (MY_ROLE == NIL) { //If I am dead, turn off my light
      setColor(OFF);
    } else {
      setColorOnFace(MY_COLOR, idFace);
    }

    if (HAVE_PLAYER_CONNECTED) {
      //A player just connected - get their ID and act accordingly
      connectedPlayerId = getLastValueReceivedOnFace(idFace) - PLAYER_ID_START_VAL;

      //Check for role to act accordingly
      switch (roles[connectedPlayerId]) {
        case EVIL:
          isKilled = true;
          //Show that I am killed
          FOREACH_FACE(f) {
            if (f != idFace) {
              setColorOnFace(ROLE_COLORS[EVIL], f);
            }
          }
          break;
        case WIZARD:
          isProtected = true;
          //Show that I am protected
          FOREACH_FACE(f) {
            if (f != idFace) {
              setColorOnFace(ROLE_COLORS[WIZARD], f);
            }
          }
          break;
        case KNIGHT:
          //Reveal my role
          FOREACH_FACE(f) {
            if (f != idFace) {
              setColorOnFace(MY_ROLE_COLOR, f);
            }
          }
          break;
        default:
          //Show that the player is Normal
          FOREACH_FACE(f) {
            if (f != idFace) {
              setColorOnFace(ROLE_COLORS[NORMAL], f);
            }
          }
          break;
      }
    } else {
      FOREACH_FACE(f) {
        if (f != idFace && f != pointerFace) {
          setColorOnFace(OFF, f);
        }
      }
    }

    //Propagate ID value or move to voting mode
    setValueSentOnFace(propActionVal, nextFace);
  }
}

void votingPhaseLoop() {
  if (type == BOARD) {
    //Reset action phase values
    propActionVal = 0;
    roundStarted = false;
    isTurnDone = false;
    isMyTurn = false;
    isActionRoot = false;

    //Reset results phase values
    numVotes = 0;
    resultsDone = false;
    resultsStarted = false;
    isResultsRoot = false;

    //Propagate mode
    if (isVotingRoot) {
      if (!votingStarted) {
        setValueSentOnFace(VOTING_PHASE_MODE, nextFace);
        if (getLastValueReceivedOnFace(prevFace) == VOTING_PHASE_MODE) {
          votingStarted = true;
          setValueSentOnFace(EMPTY, nextFace);
        }
      }
    } else {
      if (getLastValueReceivedOnFace(prevFace) == VOTING_PHASE_MODE) {
        setValueSentOnFace(VOTING_PHASE_MODE, nextFace);
      } else {
        setValueSentOnFace(EMPTY, nextFace);
      }
    }

    if (IS_ALIVE(id)) {
      //Display ID color
      setColorOnFace(MY_COLOR, idFace);

      //Display kill and protection status
      if (isKilled) {
        setColorOnFace(ROLE_COLORS[EVIL], (idFace + 1) % FACE_COUNT);
      }
      if (isProtected) {
        setColorOnFace(ROLE_COLORS[WIZARD], (idFace - 1) % FACE_COUNT);
      }

      //Put Players into VOTING_PHASE_MODE so they count votes
      setValueSentOnFace(VOTING_PHASE_MODE, idFace);
    }

    //Watch for button press to enter results mode
    if (buttonSingleClicked()) {
      resultsStarted = false;
      isResultsRoot = true;
      setValueSentOnFace(RESULTS_PHASE_MODE, nextFace);
      changeMode(RESULTS_PHASE_MODE);
    }
  } else if (type == PLAYER) {
    //Set color
    setColor(MY_COLOR);

    //Propagate mode
    setValueSentOnFace(VOTING_PHASE_MODE, pointerFace);

    //Get count from behind, increment, and propagate forward to board
    if (!isValueReceivedOnFaceExpired(pointerFace)) {
      byte val = getLastValueReceivedOnFace(pointerFace);
      setValueSentOnFace(val + 1, idFace);
    } else {
      setValueSentOnFace(PLAYER_COUNTER_START_VAL, idFace);
    }
  }
}

void resultsPhaseLoop() {
  if (type == BOARD) {
    //Reset voting phase vals
    votingStarted = false;

    //Keep telling the PLAYERS pieces that we're voting
    setValueSentOnFace(VOTING_PHASE_MODE, idFace);

    //Get the number of votes
    numVotes = 0;
    if(!isValueReceivedOnFaceExpired(idFace)){
      numVotes = getLastValueReceivedOnFace(idFace) - PLAYER_COUNTER_START_VAL + 1;
    }


    //Kill me if needed
    if (isKilled && !isProtected) {
      killSelf();
    }
    isKilled = false;
    isProtected = false;

    //Get the val from prev face
    byte prevCount = getLastValueReceivedOnFace(prevFace);

    //Propagate mode
    if (!resultsDone) {
      if (isResultsRoot) {
        if (!resultsStarted) {
          setValueSentOnFace(RESULTS_PHASE_MODE, nextFace);
          if (getLastValueReceivedOnFace(prevFace) == RESULTS_PHASE_MODE) {
            resultsStarted = true;

            //Step 1 as Root: Begin counting
            if (!IS_ALIVE(id)) {
              setValueSentOnFace(PLAYER_COUNTER_START_VAL, nextFace);
            } else {
              setValueSentOnFace(PLAYER_COUNTER_START_VAL + 1, nextFace);
            }
          }
        } else {
          //Step 2 as Root: I now know what the number of living players is, and must tell the others
          if (prevCount >= PLAYER_COUNTER_START_VAL && prevCount <= PLAYER_COUNTER_START_VAL + 7) {
            numPlayers = prevCount - PLAYER_COUNTER_START_VAL;
            setValueSentOnFace(numPlayers + NUM_PLAYERS_START_VAL, nextFace);
          }

          //Step 3 as Root: I now know that everyone knows the number of living players
          else if (prevCount >= NUM_PLAYERS_START_VAL && prevCount <= NUM_PLAYERS_START_VAL + 6) {
            resultsDone = true;
          }
        }
      } else {
        if (getLastValueReceivedOnFace(prevFace) == RESULTS_PHASE_MODE) {
          setValueSentOnFace(RESULTS_PHASE_MODE, nextFace);
        } else {
          //Step 1 as Non-Root: Listen for counter values
          if (prevCount >= PLAYER_COUNTER_START_VAL && prevCount <= PLAYER_COUNTER_START_VAL + 7) {
            //Continue the count
            if (!IS_ALIVE(id)) {
              setValueSentOnFace(prevCount, nextFace);
            } else {
              setValueSentOnFace(prevCount + 1, nextFace);
            }
          }

          //Step 2 as Non-Root: Listen for the number of living players
          else if (prevCount >= NUM_PLAYERS_START_VAL && prevCount <= NUM_PLAYERS_START_VAL + 6) {
            numPlayers = prevCount - NUM_PLAYERS_START_VAL;
            setValueSentOnFace(prevCount, nextFace); //Propagate the number of living players
            resultsDone = true;
          }
        }
      }
    } else {
      if (IS_ALIVE(id)) {
        setColorOnFace(MY_COLOR, idFace);
        switch(numPlayers){
          case 6:
            if(numVotes > 3){
              killSelf();
              numPlayers--;
            }
          break;

          case 5:
          case 4:
            if(numVotes > 2){
              killSelf();
              numPlayers--;
            }
          break;

          case 3:
            if(numVotes > 1){
              killSelf();
              numPlayers--;
            }
          break;
          default:
            setValueSentOnAllFaces(LOSS_MODE);
          break;
        }
      } else {
        setColor(OFF);
      }

      //Results are done - wait for click to proceed
      if (buttonSingleClicked()) {
        //Move to action phase
        roundStarted = false;
        setColor(WHITE);
        isActionRoot = true;
        setValueSentOnFace(ACTION_PHASE_MODE, nextFace);
        changeMode(ACTION_PHASE_MODE);
      }
    }
  }
}

void lossLoop() {
  setValueSentOnAllFaces(LOSS_MODE);
  setColor(RED);
}

void victoryLoop() {
  setValueSentOnAllFaces(VICTORY_MODE);
  setColor(GREEN);
}

void killSelf() {
  if (MY_ROLE == EVIL) {
    setValueSentOnAllFaces(VICTORY_MODE);
  }
  roles[id] = NIL;
}

void changeMode(MODE mode) {
  setColor(OFF);
  currentMode = mode;
}
