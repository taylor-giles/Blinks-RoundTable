Color COLORS[] = {OFF, RED, ORANGE, YELLOW, GREEN, BLUE, MAGENTA};
Color ROLE_COLORS[] = { OFF, BLUE, GREEN, RED, YELLOW };
enum MODE {
  EMPTY=0,
  IDLE_MODE,
  SETUP_MODE,
  COLOR_CHOOSING_MODE,
  ROLE_CHOOSING_MODE,
  ACTION_PHASE_MODE,
  VOTING_PHASE_MODE,
  RESULTS_PHASE_MODE,
  GAME_OVER_MODE,
  NUM_MODES
} currentMode = IDLE_MODE;

const byte BOARD_ID_START_VAL = NUM_MODES + 1; //Must be greater than NUM_MODES
const byte PLAYER_ID_START_VAL = BOARD_ID_START_VAL + 8; //Must be at least 7 greater than BOARD_ID_START_VAL
const byte PLAYER_COUNTER_START_VAL = PLAYER_ID_START_VAL + 8; //Must be greater than NUM_MODES
const byte NUM_PLAYERS_START_VAL = PLAYER_COUNTER_START_VAL + 8; //Must be at least 7 greater than PLAYER_COUNTER_START_VAL
bool isModeRoot = false;

// Role destribution constants
enum ROLES_DISTRIBUTION_VALS {
  ROLE_BASE = NUM_PLAYERS_START_VAL + 8,
  
  WHOS_NEXT_PLAYER,
  NEXT_PLAYER_START,
  
  NEXT_ROLE_START = NEXT_PLAYER_START + 7,
  
  ROLE_KNIGHT = NEXT_ROLE_START + 7,
  ROLE_WIZARD,
  ROLE_EVIL,
  ROLE_NORMAL,
};
const ROLES_DISTRIBUTION_VALS fourPlayers[] = { ROLE_NORMAL, ROLE_KNIGHT, ROLE_WIZARD, ROLE_EVIL };
const ROLES_DISTRIBUTION_VALS fivePlayers[] = { ROLE_NORMAL, ROLE_WIZARD, ROLE_EVIL, ROLE_NORMAL, ROLE_KNIGHT };
const ROLES_DISTRIBUTION_VALS sixPlayers[] = { ROLE_NORMAL, ROLE_EVIL, ROLE_KNIGHT, ROLE_NORMAL, ROLE_NORMAL, ROLE_WIZARD };

enum ROLE_TYPE { NIL=0, KNIGHT, WIZARD, EVIL, NORMAL, PENDING };

//Data for this Blink
enum PIECE_TYPE { BOARD, PLAYER } type = BOARD;
byte id = 0; //Valid ID range: 1-6
byte idFace = 0; //Face on which the color for this Blink should be shown
byte pointerFace = 0; //Face opposite the idFace
byte nextFace;
byte prevFace;
byte numPlayers;
bool isDeaf = false;
ROLE_TYPE roles[] = {NIL, NIL, NIL, NIL, NIL, NIL, NIL};  // Stores the roles of all the blinks
bool isRoleDisplayed = false; //true if the player's role is currently being displayed on this Blink
byte currentPlayerId = 0; //The ID of the player who is currently taking a turn
bool isKilled = false;
bool isProtected = false;
byte connectedPlayerId = 0;
byte numVotes = 0;
byte prevNextRole = 0;
byte prevRole = 0;

//Roles distribution vars
byte num_players_counted = 0;
byte last_sent_role_index = 7;
byte which_id_accepting_role = 0;

Timer resultsTimer;

#define HAVE_PLAYER_CONNECTED (!isValueReceivedOnFaceExpired(idFace))
#define HAS_PLAYER_CONNECTED(id) (roles[id] != NIL)
#define MY_COLOR (COLORS[id])
#define MY_ROLE (roles[id])
#define MY_ROLE_COLOR (ROLE_COLORS[MY_ROLE])

void setup() {
  // put your setup code here, to run once:
  randomize();
}

void loop() {
  //Reset on long press
  if(buttonLongPressed()){ //NOTE This cannot be the same as the top condition in setupLoop()
    id = 0;
    setValueSentOnAllFaces(EMPTY);
    type = BOARD;
    currentMode = IDLE_MODE;
    setColor(OFF);
    roles[0] = NIL;
    roles[1] = NIL;
    roles[2] = NIL;
    roles[3] = NIL;
    roles[4] = NIL;
    roles[5] = NIL;
    roles[6] = NIL;
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
  }
  
  //If already set up...
  if(id > 0){
    checkForNewMode();
  
    switch(currentMode){
      case COLOR_CHOOSING_MODE:
        colorChoosingLoop();
      break;

      case ROLE_CHOOSING_MODE:
        roleChoosingLoop();
      break;
        
      case ACTION_PHASE_MODE:
        actionPhaseLoop();
      break;
    }
  } 

  //Do setup
  else {
    setupLoop();
  }
}

void setupLoop(){
  //If button is double clicked, start off chain
  if(buttonDoubleClicked()){ //NOTE This cannot be the same as the top condition for setup in loop()
    //Look for an available face to send ID to
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        //Send ID 1
        setValueSentOnFace(BOARD_ID_START_VAL + 1, f);
        nextFace = f;
        setColorOnFace(BLUE, f);
        break;
      }
    }
  }
  
  //Listen for an ID
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      byte val = getLastValueReceivedOnFace(f);
      if(val > BOARD_ID_START_VAL && val <= BOARD_ID_START_VAL + FACE_COUNT){ //Make sure the 1-offset for the IDs is accounted for
        //ID is found, and this is a board Blink
        type = BOARD;
        id = val - BOARD_ID_START_VAL;
        prevFace = f;
        setColorOnFace(GREEN, f);

        if(id < FACE_COUNT){ //The root will always get its ID last, so it will be equal to FACE_COUNT
          //Look for an available face to send ID to
          FOREACH_FACE(f){
            if(!isValueReceivedOnFaceExpired(f) && f != prevFace){
              //Send next ID
              setValueSentOnFace(val + 1, f);
              nextFace = f;
              setColorOnFace(BLUE, f);
            }
          }
        }
        setColor(OFF);
        changeMode(COLOR_CHOOSING_MODE);
      } else if(val > PLAYER_ID_START_VAL && val <= PLAYER_ID_START_VAL + FACE_COUNT + 1){ //Make sure the 1-offset for the IDs is accounted for
        //ID is found, and this is a player Blink
        type = PLAYER;
        id = val - PLAYER_ID_START_VAL;
        idFace = f;
        pointerFace = (idFace + 3) % FACE_COUNT;
        changeMode(ROLE_CHOOSING_MODE);
        setColorOnFace(COLORS[id], idFace);
      }
    }
  }
}

/**
 * Look at prevFace for mode change information
 */
MODE checkForNewMode(){
  if(!isValueReceivedOnFaceExpired(prevFace)){
    byte value = getLastValueReceivedOnFace(prevFace);

    //Look for a mode that matches
    for(byte m = IDLE_MODE; m < NUM_MODES; m++){
      if(value == m && currentMode != m){
        changeMode(m);
      }
    }
  }

  //Go into action phase mode if I am a player Blink and alone
  if(type == PLAYER && isAlone() && currentMode != ACTION_PHASE_MODE){
    changeMode(ACTION_PHASE_MODE);
  }
}

void colorChoosingLoop(){
  //Continue to send id to next Blink
  setValueSentOnFace(id + BOARD_ID_START_VAL + 1, nextFace);

  //Calculate the outward-facing face of this Blink
  idFace = (nextFace + 2) % FACE_COUNT;
  if(idFace == prevFace){
    idFace = (prevFace + 2) % FACE_COUNT;
  }
  pointerFace = (idFace + 3) % FACE_COUNT;
  
  //Set this Blink's color
  setColorOnFace(COLORS[id], idFace);

  //Send ID to player Blinks
  setValueSentOnFace(id + PLAYER_ID_START_VAL, idFace);

  //Listen for single-press to indicate game start
  if(buttonSingleClicked()){
    isModeRoot = true;
    setValueSentOnFace(ROLE_CHOOSING_MODE, nextFace);
    changeMode(ROLE_CHOOSING_MODE);
  }
}

void roleChoosingLoop(){
  if(type == BOARD){
    setValueSentOnFace(ROLE_CHOOSING_MODE, nextFace); //Propagate mode
    setColorOnFace(COLORS[id], idFace); //Display ID color
    byte prevCount = getLastValueReceivedOnFace(prevFace); //Get the value on prevFace
    if(isModeRoot){
      //Step 1 as Root: Begin the count
      if(prevCount == ROLE_CHOOSING_MODE && !isDeaf){ //isDeaf: Stop listening for ROLE_CHOOSING_MODE input once you send the first PLAYER_COUNTER value
        //Begin counting
        if(isValueReceivedOnFaceExpired(idFace)){
          setValueSentOnFace(PLAYER_COUNTER_START_VAL, nextFace);
        } else {
          setValueSentOnFace(PLAYER_COUNTER_START_VAL + 1, nextFace);
        }
      }
  
      //Step 2 as Root: I now know what the number of players is, and must tell the others
      else if(prevCount >= PLAYER_COUNTER_START_VAL && prevCount <= PLAYER_COUNTER_START_VAL + 6){
        isDeaf = true; //Now I should stop accepting ROLE_CHOOSING_MODE input
        numPlayers = prevCount - PLAYER_COUNTER_START_VAL;
        setValueSentOnFace(numPlayers + NUM_PLAYERS_START_VAL, nextFace);
      }
  
      //Step 3 as Root: I now know that everyone knows the number of players, so roles distribution can begin
      else if(prevCount >= NUM_PLAYERS_START_VAL && prevCount <= NUM_PLAYERS_START_VAL + 6){
        setColor(YELLOW);

        //Distribute roles - figure out who is player 1
        setValueSentOnFace(WHOS_NEXT_PLAYER, nextFace);
        prevNextRole = 0;
        prevRole = 0;
      }  
      
      //Step 4 as Root: Listen for NEXT_PLAYER signal to determine the ID of the next player
      else if(prevCount >= NEXT_PLAYER_START && prevCount <= NEXT_PLAYER_START + 6) {
        setColor(ORANGE);
        //Set role of new player to PENDING
        roles[prevCount - NEXT_PLAYER_START] = PENDING;
        
        //Ask for ID of next player
        setValueSentOnFace(WHOS_NEXT_PLAYER, nextFace);
      }
      
      //Step 5 as Root: I know the IDs of all players. I will start sending out the roles.
      else if(prevCount == WHOS_NEXT_PLAYER){
        setColor(GREEN);
        // Check ourself 
        if(HAVE_PLAYER_CONNECTED) {
          roles[id] = PENDING;
        }
        
        //Send the ID for the first role
        for(byte i = 1; i < 6; i++) {
            if(roles[i] == PENDING) { 
              setColor(CYAN);
              setValueSentOnFace(NEXT_ROLE_START + i, nextFace);
              break;
            }
         }
      }
      
      //Step 6 as Root: The ID I just sent out has been read. Now I should send out the role for that ID
      else if(prevCount >= NEXT_ROLE_START && prevCount <= NEXT_ROLE_START + 6 && prevCount > prevNextRole) { 
        setColor(BLUE);
        //Update prevNextRole
        prevNextRole = prevCount;
        
        // Send out the actual role now
        
        // if = 7 then we have not decided what role to start with
        if(last_sent_role_index == 7) {
          // TODO: Add randomness here
          last_sent_role_index = 2;
        }
        
        //Determine which ID will be taking this role (obtained from response value)
        which_id_accepting_role = prevCount - NEXT_ROLE_START;
        
        //Determine which role to assign to this ID
        ROLES_DISTRIBUTION_VALS next;
        switch(numPlayers) {
          default:
          case 4:
            next = fourPlayers[(last_sent_role_index + 1) % 4];
            last_sent_role_index = (last_sent_role_index + 1) % 4;
            break;
          case 5:
            next = fivePlayers[ (last_sent_role_index + 1) % 5 ];
            last_sent_role_index = (last_sent_role_index + 1) % 5;
            break;
          case 6:
            next = sixPlayers[(last_sent_role_index + 1) % 6];
            last_sent_role_index = (last_sent_role_index + 1) % 6;
            break;
        }
        
        //Update my local Roles array
        if(next == ROLE_KNIGHT)
          roles[which_id_accepting_role] = KNIGHT;
        else if(next == ROLE_EVIL)
          roles[which_id_accepting_role] = EVIL;
        else if(next == ROLE_NORMAL)
          roles[which_id_accepting_role] = NORMAL;
        else if(next == ROLE_WIZARD)
          roles[which_id_accepting_role] = WIZARD;
        
        //Send the role to the others
        setValueSentOnFace(next, nextFace);
      }
      
      
      //Step 7 as Root: The role I just sent out has been read. Now I should send out the ID for the next role
      else if( prevCount >= ROLE_KNIGHT && prevCount <= ROLE_NORMAL && prevCount > prevRole) { 
        setColor(MAGENTA);
        prevRole = prevCount;
        // curr sent out role is in variable curr_sent_out_role
        
        // which_id_accepting_role is the last id which recieved a role
        
        byte i;
        for(i = 1; i <= 6; i++) {
          if(roles[i] == PENDING) {
            setValueSentOnFace(NEXT_ROLE_START + i, nextFace);
            break;
          }
        }
        
        // If we have assigned all the roles
        if(i >= 6) {
          setColor(RED);
        }
      }
    } else {
      //Step 1 as Non-Root: Listen for counter values
      if(prevCount >= PLAYER_COUNTER_START_VAL && prevCount <= PLAYER_COUNTER_START_VAL + 6){
        //Propagate the count
        if(isValueReceivedOnFaceExpired(idFace)){
          setValueSentOnFace(prevCount, nextFace);
        } else {
          setValueSentOnFace(prevCount + 1, nextFace);
        }
      } 
    
      //Step 2 as Non-Root: Listen for the number of players
      else if(prevCount >= NUM_PLAYERS_START_VAL && prevCount <= NUM_PLAYERS_START_VAL + 6){
        numPlayers = prevCount - NUM_PLAYERS_START_VAL;
        setValueSentOnFace(prevCount, nextFace); //Propagate the number of players
        FOREACH_FACE(f){
          if(f < numPlayers){
            setColorOnFace(WHITE, f);
          }
        }
      }

      //Step 3 as Non-Root: Listen for WHOS_NEXT_PLAYER and respond if I am accepting the role of next player
      else if(prevCount >= WHOS_NEXT_PLAYER && prevCount <= NEXT_PLAYER_START + 6) {
        // See if we have a player connected      
        if(HAVE_PLAYER_CONNECTED && prevCount == WHOS_NEXT_PLAYER) {
          if(roles[id] == NORMAL){
            setValueSentOnFace(WHOS_NEXT_PLAYER, nextFace);
          } else {
            //Claim the role
            roles[id] = NORMAL;
            setValueSentOnFace(NEXT_PLAYER_START + id, nextFace);
          }
        } else {
          setValueSentOnFace(WHOS_NEXT_PLAYER, nextFace);
        }
      }
      
      //Step 4 as Non-Root: Listen for the ID that the root wants to update the role for, and set it to PENDING
      else if(prevCount >= NEXT_ROLE_START && prevCount <= NEXT_ROLE_START + 6) {
        roles[prevCount - NEXT_ROLE_START] = PENDING;
  
        // Pass along the value
        setValueSentOnFace(prevCount, nextFace);
      }

      //Step 5 as Non-Root: Listen for the role that the root wants to assign to the PENDING ID
      else if(prevCount >= ROLE_KNIGHT && prevCount <= ROLE_NORMAL) {
        // Find the pending role and replace it with this role
        for(byte i = 1; i <= 6; i++) {
          if(roles[i] == PENDING) {
            switch(prevCount) {
              case ROLE_KNIGHT:
                roles[i] = KNIGHT;
                break;
              case ROLE_WIZARD:
                roles[i] = WIZARD;
                break;
              case ROLE_EVIL:
                roles[i] = EVIL;
                break;
              default:
              case ROLE_NORMAL:
                roles[i] = NORMAL;
                break;
            }
            break;
          }
        }
        
        // Pass along the value
        setValueSentOnFace(prevCount, nextFace);
      }
    }
  } else if(type == PLAYER){
    //Listen for a role value
    
  }
}

void actionPhaseLoop(){
  if(type == PLAYER){
    //Broadcast my ID on my idFace
    setValueSentOnFace(EMPTY, pointerFace);
    setValueSentOnFace(id + PLAYER_ID_START_VAL, idFace);
    setColorOnFace(COLORS[id], idFace);
    
    //Listen for single click to toggle role color display
    if(buttonSingleClicked()){
      isRoleDisplayed = !isRoleDisplayed;
    }
    
    //Toggle role color display
    if(isRoleDisplayed){
      FOREACH_FACE(f){
        if(f != idFace){
          setColorOnFace(MY_ROLE_COLOR, f);
        }
      }
    } else {
      FOREACH_FACE(f){
        if(f != idFace){
          setColorOnFace(OFF, f);
        }
      }
    }
  } else if(type == BOARD){
    //Listen for new ID on board
    byte newId = getLastValueReceivedOnFace(prevFace) - BOARD_ID_START_VAL;
    
    //New turn just started
    if(newId > currentPlayerId){
      connectedPlayerId = 0;
      currentPlayerId = newId;
      isKilled = false;
      isProtected = false;
    }
    
    //Skip this ID if it doesnt have a player attached
    if(!HAS_PLAYER_CONNECTED(id)){ //Checks if the given ID has a player associated with it
      setColor(OFF);
      currentPlayerId++;
    } else {
      setColorOnFace(MY_COLOR, idFace);
      if(currentPlayerId == id){
        //My player is currently taking a turn
        setColorOnFace(WHITE, pointerFace);
      } 
      
      //A player is currently taking a turn
      //Listen for a player ID
      if(HAVE_PLAYER_CONNECTED){ //Checks if this board Blink has a player Blink physically connected to it
        connectedPlayerId = getLastValueReceivedOnFace(prevFace) - PLAYER_ID_START_VAL;
      } else if(connectedPlayerId > 0){
        //There was a player connected, and it was removed. Move to the next player's turn
        //Clear display
        FOREACH_FACE(f){
          if(f != idFace){
            setColorOnFace(OFF, f);
          }
        }
        currentPlayerId++;
      }
      
      //If the connected player is the player currently taking a turn, act accordingly
      if(connectedPlayerId > 0 && connectedPlayerId == currentPlayerId){
        switch(roles[connectedPlayerId]){
          case EVIL:
            isKilled = true;
            //Show that I am killed
            FOREACH_FACE(f){
              if(f != idFace){
                setColorOnFace(ROLE_COLORS[EVIL], f);
              }
            }
          break;
          case WIZARD:
            isProtected = true;
            //Show that I am protected
            FOREACH_FACE(f){
              if(f != idFace){
                setColorOnFace(ROLE_COLORS[WIZARD], f);
              }
            }
          break;
          case KNIGHT:
            //Reveal my role
            FOREACH_FACE(f){
              if(f != idFace){
                setColorOnFace(MY_ROLE_COLOR, f);
              }
            }
          break;
          default:
            //Show I am normal
            FOREACH_FACE(f){
              if(f != idFace){
                setColorOnFace(ROLE_COLORS[NORMAL], f);
              }
            }
          break;
        }
      }
    }
    //Propagate ID broadcast
    setValueSentOnFace(currentPlayerId + BOARD_ID_START_VAL, nextFace);
  }
}

void votingPhaseLoop(){
  if(type == BOARD){
    //Display kill and protection status
    if(isKilled){
      setColorOnFace(ROLE_COLORS[EVIL], (idFace + 1) % FACE_COUNT);
    }
    if(isProtected){
      setColorOnFace(ROLE_COLORS[WIZARD], (idFace - 1) % FACE_COUNT);
    }

    //Ask for votes by putting Players into VOTING_PHASE_MODE
    setValueSentOnFace(VOTING_PHASE_MODE, idFace);

    //Watch for button press to enter results mode
    if(buttonSingleClicked()){
      if(!isValueReceivedOnFaceExpired(idFace)){
        numVotes = getLastValueReceivedOnFace(idFace) - PLAYER_COUNTER_START_VAL;
      } else {
        numVotes = 0;
      }
      isModeRoot = true;
      setValueSentOnFace(RESULTS_PHASE_MODE, nextFace);
      changeMode(RESULTS_PHASE_MODE);
    }
  } else if(type == PLAYER){
    //Set color
    setColor(MY_COLOR);
    
    //Propagate mode
    setValueSentOnFace(VOTING_PHASE_MODE, pointerFace);

    //Get count from behind, increment, and propagate forward to board
    if(!isValueReceivedOnFaceExpired(pointerFace)){
      byte val = getLastValueReceivedOnFace(pointerFace);
      setValueSentOnFace(val + 1, idFace);
    } else {
      setValueSentOnFace(PLAYER_COUNTER_START_VAL, idFace);
    }
  }
}

void resultsPhaseLoop(){
  if(type == BOARD){
    setValueSentOnFace(RESULTS_PHASE_MODE, nextFace); //Propagate mode
    byte prevCount = getLastValueReceivedOnFace(prevFace); //Get the value on prevFace
    if(isModeRoot){
      //Step 1 as Root: Begin the count
      if(prevCount == RESULTS_PHASE_MODE && !isDeaf){ //isDeaf: Stop listening for ROLE_CHOOSING_MODE input once you send the first PLAYER_COUNTER value
        //Begin counting
        if(isValueReceivedOnFaceExpired(idFace)){
          setValueSentOnFace(PLAYER_COUNTER_START_VAL, nextFace);
        } else {
          setValueSentOnFace(PLAYER_COUNTER_START_VAL + 1, nextFace);
        }
      }
  
      //Step 2 as Root: I now know what the number of living players is, and must tell the others
      else if(prevCount >= PLAYER_COUNTER_START_VAL && prevCount <= PLAYER_COUNTER_START_VAL + 6){
        isDeaf = true; //Now I should stop accepting RESULTS_PHASE_MODE input
        numPlayers = prevCount - PLAYER_COUNTER_START_VAL;
        setValueSentOnFace(numPlayers + NUM_PLAYERS_START_VAL, nextFace);
      }
  
      //Step 3 as Root: I now know that everyone knows the number of living players
      else if(prevCount >= NUM_PLAYERS_START_VAL && prevCount <= NUM_PLAYERS_START_VAL + 6){
        setColor(YELLOW);

      }
    } else {
      //Step 1 as Non-Root: Listen for counter values
      if(prevCount >= PLAYER_COUNTER_START_VAL && prevCount <= PLAYER_COUNTER_START_VAL + 6){
        //Propagate the count
        if(isValueReceivedOnFaceExpired(idFace)){
          setValueSentOnFace(prevCount, nextFace);
        } else {
          setValueSentOnFace(prevCount + 1, nextFace);
        }
      } 
    
      //Step 2 as Non-Root: Listen for the number of living players
      else if(prevCount >= NUM_PLAYERS_START_VAL && prevCount <= NUM_PLAYERS_START_VAL + 6){
        numPlayers = prevCount - NUM_PLAYERS_START_VAL;
        setValueSentOnFace(prevCount, nextFace); //Propagate the number of living players
        FOREACH_FACE(f){
          if(f < numPlayers){
            setColorOnFace(WHITE, f);
          }
        }
      }
    }
  } else if(type == PLAYER){
    //Listen for a role value
    
    
  }
}

void changeMode(MODE mode){
  setColor(OFF);
  currentMode = mode;
}
