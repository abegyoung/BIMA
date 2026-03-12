// prototyping -- these live in Commands.c

extern int nop( int, char **, int, int );
extern int runSweep( int, char **, int, int );
extern int setBias( int, char **, int, int );
extern int setLNAdrain( int, char **, int, int );
extern int setLNAgate( int, char **, int, int );
extern int setIF( int, char **, int, int );
extern int setFeedback( int, char **, int, int );
extern int doVgap( int, char **, int, int );
extern int setTTL( int, char **, int, int );
extern int setMonitor( int, char **, int, int );
extern int setDAC( int, char **, int, int );
extern int setCanOut( int, char **, int, int );
extern int setLband( int, char **, int, int );
extern int setFreq( int, char **, int, int );
extern int doBump( int, char **, int, int );
extern int sendCan( int, char **, int, int );
extern int sendStatus ( int, char **, int, int );



// command structure definition
struct cmd commands[] =
{
  {
    "status", &sendStatus,
    "send the server struct status",
    0,
    {
      { "", 0, 0, {} }
    },
  },

  {
    "can", &setCanOut,
    "set the can output file descriptor",
    0,
    {
      { "", 0, 0, {} }
    },
  },

  {
    "setLband", &setLband,
    "set the can output file descriptor",
    4,
    {
      { "band", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"1","3"} },
      { "YIGHarmonicN", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"8","9"} },
      { "GunnHarmonicM", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"8","10"} },
      { "freq", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"1100","1260"} },
    },
  },
  {
    "setfreq", &setFreq,
    "set the can output file descriptor",
    1,
    {
      { "freq", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"80","273"} },
    },
  },
  {
    "sendcan", &sendCan,
    "Write a message to the CAN bus",
    2,
    {
      { "msgid", CMDARR_ARGF_REQ, 0, {} },
      { "data", CMDARR_ARGF_REQ, 0, {} },
    },
  },
  {
    "bump", &doBump,
    "bump a motor FWD or REV",
    2,
    {
      { "chan", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","2"} },
      { "direction", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","1"} },
    },
  },

  {
    "sweep", &runSweep,
    "do a IV sweep",
    4,
    {
      { "start", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","15000"} },
      { "stop", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","15000"} },
      { "step", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","1000"} },
      { "power", CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","1"} },
    },
  },

  {
    "monitor", &setMonitor,
    "set the CAN Blanking Frames",
    1,
    {
      { "mask", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","255"} },
    },
  },

  {
    "dcdc", &setTTL,
    "set the DCDC TTL registers",
    1,
    {
      { "mask", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","255"} },
    },
  },

  {
    "setfeedback", &setFeedback,
    "set the constant v-mode registers",
    1,
    {
      { "mask", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","255"} },
    },
  },

  {
    "setIF", &setIF,
    "set IF Attens for IF Total Power",
    1,
    {
      { "val", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","24000"} },
    },
  },

  {
    "setdac", &setDAC,
    "set DAC value",
    2,
    {
      { "chan", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","3"} },
      { "dac", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","32768"} },
    },
  },

  {
    "setbias", &setBias,
    "set mixer bias via DAC value",
    1,
    {
      { "dac", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","24000"} },
    },
  },


  {
    "setLNAdrain", &setLNAdrain,
    "set LNA drain Voltage",
    1,
    {
      { "dac", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","24000"} },
    },
  },

  {
    "setLNAgate", &setLNAgate,
    "set LNA gate Voltage",
    1,
    {
      { "dac", CMDARR_ARGF_REQ | CMDARR_ARGF_NUM | CMDARR_ARGF_RANGE, 2, {"0","24000"} },
    },
  },

  {
    "doVgap", &doVgap,
    "Find the junction gap voltage",
    0,
    {
      { "", 0, 0, {} }
    },
  },

  {
    "help", &nop,
    "Displays a list of commands",
    0,
    {
      { "", 0, 0, {} }
    },
  },
  {
    "?", &nop,
    "Synonym for 'help'",
    0,
    {
      { "", 0, 0, {} }
    },
  },
  {
    "quit", &nop,
    "Quit server",
    0,
    {
      { "", 0, 0, {} }
    },
  },
  {
    "echo", &nop,
    "Echo string to user, for watchdog.",
    1,
    {
      { "say", CMDARR_ARGF_REQ, 0, {} }
    },
  },
  {
    "", NULL,
    "",
    0,
    {
      { "", 0, 0, {} }
    },
  },
};

