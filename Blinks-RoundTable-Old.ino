// Pre-Defines Colors
static Color colors[6] = {RED, ORANGE, YELLOW, GREEN, CYAN, BLUE};

enum PIECE_TYPE { BOARD, PLAYER };
enum ROLE_TYPE { KNIGHT, WIZARD, EVIL, NORMAL };
enum MODE {
  EMPTY=0,
  PRE_SETUP_MODE=1,
  SETUP_PLAYER_MODE=2,
  COLOR_CHOOSING_MODE=3,
  ACTION_PHASE_MODE=4,
  VOTING_PHASE_MODE=5,
  GAME_OVER_MODE=6,
  SETUP_BOARD_MODE=7,
} currentMode = PRE_SETUP_MODE;

#define BOARD_SETUP_START_NUM 10

static ROLE_TYPE fourPlayers[4] = { NORMAL, KNIGHT, WIZARD, EVIL };
static ROLE_TYPE fivePlayers[5] = { NORMAL, WIZARD, EVIL, NORMAL, KNIGHT };
static ROLE_TYPE sixPlayers[6] = {NORMAL, EVIL, KNIGHT, NORMAL, NORMAL, WIZARD};

static struct PIECE {
  PIECE_TYPE type;
  byte idFace; //The face (LED wedge) on which this Blink’s ID color will be displayed

  // num: 0-5 (Player number and color (via index of colors Array))
  byte num;

  //Board variables - the face on which the next/previous board piece is connected
  byte nextPieceFace;
  byte prevPieceFace;
} pieceData;

bool isModeRoot = false; //True if this Blink is the one which started Setup Mode
Timer modeDelayTimer;

void setup() {
  resetPieceData();
  randomize();
}

void loop() {
  //If I'm done screaming, listen for updates
  if(modeDelayTimer.isExpired()){
    checkFaces();
  }
  
  //If the button was long pressed, enter Board Setup mode
  if(buttonLongPressed()){
    resetPieceData();
    causeModeChange(SETUP_BOARD_MODE);
  }

  //Perform different actions in each mode
  switch(currentMode){
    case SETUP_BOARD_MODE:
      initBoard();   
    break;

    case COLOR_CHOOSING_MODE:
      if(pieceData.type == BOARD){
        chooseColorsBoard();
      }
    break;
  }
}

/**
 * Reads the values of all the faces on this Blink and acts accordingly
 */
void checkFaces(){
  FOREACH_FACE(f){
    byte valueOnFace = getLastValueReceivedOnFace(f);

    //Check for board initialization
    if(valueOnFace == SETUP_BOARD_MODE && currentMode != SETUP_BOARD_MODE){
      resetPieceData();
      setColor(RED);
      setValueSentOnAllFaces(SETUP_BOARD_MODE);
      currentMode = SETUP_BOARD_MODE;
    } else 

    //Check for Color Choosing Mode
    if(valueOnFace == COLOR_CHOOSING_MODE && currentMode != COLOR_CHOOSING_MODE){
      setColor(BLUE);
      setValueSentOnAllFaces(COLOR_CHOOSING_MODE);
      modeDelayTimer.set(500);
      currentMode = COLOR_CHOOSING_MODE;
    }
  }
}

  
/**
 * Handles the Board Init Mode code
 */
void initBoard(){
//  setColor(BLUE);
  pieceData.type = BOARD;
  pieceData.num = 16; //Arbitrary number above 6 to act as "not yet defined" value

  //If this piece is the one which started setup mode
  if(isModeRoot){
//    setColor(RED);
    //Wait 500ms to make sure all other Blinks are in setup mode
    if(modeDelayTimer.isExpired()){
//      setColor(YELLOW);
      
      //Find the next connected piece by cycling faces
      FOREACH_FACE(f){
        //Check if another Blink is connected on this face
        if(!isValueReceivedOnFaceExpired(f)){
          //Send first setup val to the connected Blink
          setValueSentOnFace(BOARD_SETUP_START_NUM, f);
          pieceData.nextPieceFace = f;
          setColorOnFace(WHITE, f);
          break;
        }
      }
    }

    //Listen for the ending identifier
    byte faceSendingID = findFaceSendingValue(BOARD_SETUP_START_NUM + 5);
    if(faceSendingID < 7){
      pieceData.prevPieceFace = faceSendingID;

      //Setup mode is now complete - move to Color Choosing Mode
      setColor(BLUE);
      causeModeChange(COLOR_CHOOSING_MODE);
    }
  } else {
    //Listen to receive an identifier
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        byte valOnFace = getLastValueReceivedOnFace(f);
        if(valOnFace >= BOARD_SETUP_START_NUM && valOnFace <= BOARD_SETUP_START_NUM + 5){
          setColorOnFace(GREEN, f);
          pieceData.num = valOnFace - BOARD_SETUP_START_NUM;
          pieceData.prevPieceFace = f;
          
          //Find the next Blink
          FOREACH_FACE(f2){
            if(!isValueReceivedOnFaceExpired(f2) && getLastValueReceivedOnFace(f2) == SETUP_BOARD_MODE){
              //Send the next Blink its ID
              setValueSentOnFace(valOnFace + 1, f2);
              pieceData.nextPieceFace = f2;
              setColorOnFace(WHITE, f2);
            }
          }
        }
      }
    }

    //If I have my ID, send an ID to the next Blink
    if(pieceData.num < 7){
      //Find the next Blink
      FOREACH_FACE(f){
        byte valOnFace = getLastValueReceivedOnFace(f);
        if(!isValueReceivedOnFaceExpired(f) && getLastValueReceivedOnFace(f) < BOARD_SETUP_START_NUM){
          //Send the next Blink its ID
          setValueSentOnFace(pieceData.num + 1 + BOARD_SETUP_START_NUM, f);
          pieceData.nextPieceFace = f;
          setColorOnFace(WHITE, f);
        }
      }
    }
  }
}


void chooseColorsBoard(){
  if(modeDelayTimer.isExpired()){
    setColor(OFF);
    setValueSentOnAllFaces(EMPTY);
  }
  
//  setColor(colors[pieceData.num]);
//  //Calculate the outward face
//  pieceData.idFace = (pieceData.nextPieceFace + 2) % FACE_COUNT;
//  if(pieceData.idFace == pieceData.prevPieceFace){
//    pieceData.idFace = (pieceData.prevPieceFace + 2) % FACE_COUNT;
//  }
//
//  //Show this Blink's color on the ID face
//  setColorOnFace(colors[pieceData.num], pieceData.idFace);
}

void causeModeChange(byte newMode){
  isModeRoot = true;
  modeDelayTimer.set(500);
  setValueSentOnAllFaces(newMode);
  currentMode = newMode;
}

void resetPieceData(){
  pieceData.type = BOARD;
  pieceData.idFace = 0;
  pieceData.num = 0;
  pieceData.nextPieceFace = 0;
  pieceData.prevPieceFace = 0;
  isModeRoot = false;
}

/**
 * Loops through all faces looking for one receiving the given val.
 * Returns the number of the face sending the val, or 16 (arbitrary) if the val was not found (or was expired).
 */
byte findFaceSendingValue(byte val){
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f) && 
      (getLastValueReceivedOnFace(f) == val)){
        return f;
    }
  }
  return 16;
}
