Color COLORS[] = {OFF, RED, ORANGE, YELLOW, GREEN, BLUE, MAGENTA};
enum MODE {
  EMPTY=0,
  IDLE_MODE,
  SETUP_MODE,
  COLOR_CHOOSING_MODE,
  ROLE_CHOOSING_MODE,
  ACTION_PHASE_MODE,
  VOTING_PHASE_MODE,
  GAME_OVER_MODE,
  NUM_MODES
} currentMode = IDLE_MODE;

const byte SETUP_BOARD_START_VAL = 10;
const byte SETUP_PLAYER_START_VAL = 17;
const byte PLAYER_COUNTER_START_VAL = 24;
const byte NUM_PLAYERS_START_VAL = 31;
bool isModeRoot = false;

//Data for this Blink
enum PIECE_TYPE { BOARD, PLAYER } type;
byte id = 0; //Valid ID range: 1-6
byte idFace = 0; //Face on which the color for this Blink should be shown
byte pointerFace = 0; //Face opposite the idFace
byte nextFace;
byte prevFace;
byte numPlayers;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  //Reset on long press
  if(buttonLongPressed()){ //NOTE This cannot be the same as the top condition in setupLoop()
    id = 0;
    currentMode = SETUP_MODE;
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
        //Send ID
        setValueSentOnFace(SETUP_BOARD_START_VAL, f);
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
      if(val >= SETUP_BOARD_START_VAL && val <= SETUP_BOARD_START_VAL + FACE_COUNT){
        //ID is found, and this is a board Blink
        type = BOARD;
        id = val - SETUP_BOARD_START_VAL + 1;
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
      } else if(val >= SETUP_PLAYER_START_VAL && val <= SETUP_PLAYER_START_VAL + FACE_COUNT){
        //ID is found, and this is a player Blink
        type = PLAYER;
        id = val - SETUP_PLAYER_START_VAL;
        idFace = f;
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
}

void colorChoosingLoop(){
  //Continue to send id to next Blink
  setValueSentOnFace(id + SETUP_BOARD_START_VAL, nextFace);

  //Calculate the outward-facing face of this Blink
  idFace = (nextFace + 2) % FACE_COUNT;
  if(idFace == prevFace){
    idFace = (prevFace + 2) % FACE_COUNT;
  }
  pointerFace = (idFace + 3) % FACE_COUNT;
  
  //Set this Blink's color
  setColorOnFace(COLORS[id], idFace);

  //Send ID to player Blinks
  setValueSentOnFace(id + SETUP_PLAYER_START_VAL, idFace);

  //Listen for single-press to indicate game start
  if(buttonSingleClicked()){
    isModeRoot = true;
    setValueSentOnFace(ROLE_CHOOSING_MODE, nextFace);
    changeMode(ROLE_CHOOSING_MODE);
  }
}

void roleChoosingLoop(){
  if(type == BOARD){
    setColorOnFace(COLORS[id], idFace);
    byte prevCount = getLastValueReceivedOnFace(prevFace);
    if(isModeRoot){
      //Step 1 as Root: Begin the count
      if(prevCount == ROLE_CHOOSING_MODE){
        //Begin counting
        if(isValueReceivedOnFaceExpired(idFace)){
          setValueSentOnFace(PLAYER_COUNTER_START_VAL, nextFace);
        } else {
          setValueSentOnFace(PLAYER_COUNTER_START_VAL + 1, nextFace);
        }
      }
  
      //Step 2 as Root: I now know what the number of players is, and must tell the others
      else if(prevCount >= PLAYER_COUNTER_START_VAL && prevCount <= PLAYER_COUNTER_START_VAL + 6){
        numPlayers = prevCount - PLAYER_COUNTER_START_VAL;
        setValueSentOnFace(numPlayers + NUM_PLAYERS_START_VAL, nextFace);
      }
  
      //Step 3 as Root: I now know that everyone knows the number of players, so roles distribution can begin
      else if(prevCount >= NUM_PLAYERS_START_VAL && prevCount <= NUM_PLAYERS_START_VAL + 6){
        //TODO: Distribute roles
      }
    }
  
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
      FOREACH_FACE(f){
        if(f < numPlayers){
          setColorOnFace(WHITE, f);
        }
      }
    }
  }
}

void changeMode(MODE mode){
  setColor(OFF);
  currentMode = mode;
}
