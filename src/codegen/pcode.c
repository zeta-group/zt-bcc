#include "common.h"
#include "pcode.h"

enum { VARIABLE_ARGC = -1 };

// n -- number
// s -- string literal
// l -- label
// a -- array
//   g -- global
//   w -- world
//   m -- map
//   s -- script
// v -- variable
//   g -- global
//   w -- world
//   m -- map
//   s -- script
// e -- expression
// f -- function
//   e -- extension
//   u -- user
static struct pcode pcode_info[] = {
   { PCD_NONE, 0, "" },
   { PCD_TERMINATE, 0, "" },
   { PCD_SUSPEND, 0, "" },
   { PCD_PUSHNUMBER, 1, "nse" },
   { PCD_LSPEC1, 1, "ne" },
   { PCD_LSPEC2, 1, "ne" },
   { PCD_LSPEC3, 1, "ne" },
   { PCD_LSPEC4, 1, "ne" },
   { PCD_LSPEC5, 1, "ne" },
   { PCD_LSPEC1DIRECT, 2, "ne,ne" },
   { PCD_LSPEC2DIRECT, 3, "ne,ne,ne" },
   { PCD_LSPEC3DIRECT, 4, "ne,ne,ne,ne" },
   { PCD_LSPEC4DIRECT, 5, "ne,ne,ne,ne,ne" },
   { PCD_LSPEC5DIRECT, 6, "ne,ne,ne,ne,ne,ne" },
   { PCD_ADD, 0, "" },
   { PCD_SUBTRACT, 0, "" },
   { PCD_MULTIPLY, 0, "" },
   { PCD_DIVIDE, 0, "" },
   { PCD_MODULUS, 0, "" },
   { PCD_EQ, 0, "" },
   { PCD_NE, 0, "" },
   { PCD_LT, 0, "" },
   { PCD_GT, 0, "" },
   { PCD_LE, 0, "" },
   { PCD_GE, 0, "" },
   { PCD_ASSIGNSCRIPTVAR, 1, "nvse" },
   { PCD_ASSIGNMAPVAR, 1, "nvme" },
   { PCD_ASSIGNWORLDVAR, 1, "nvwe" },
   { PCD_PUSHSCRIPTVAR, 1, "nvse" },
   { PCD_PUSHMAPVAR, 1, "nvme" },
   { PCD_PUSHWORLDVAR, 1, "nvwe" },
   { PCD_ADDSCRIPTVAR, 1, "nvse" },
   { PCD_ADDMAPVAR, 1, "nvme" },
   { PCD_ADDWORLDVAR, 1, "nvwe" },
   { PCD_SUBSCRIPTVAR, 1, "nvse" },
   { PCD_SUBMAPVAR, 1, "nvme" },
   { PCD_SUBWORLDVAR, 1, "nvwe" },
   { PCD_MULSCRIPTVAR, 1, "nvse" },
   { PCD_MULMAPVAR, 1, "nvme" },
   { PCD_MULWORLDVAR, 1, "nvwe" },
   { PCD_DIVSCRIPTVAR, 1, "nvse" },
   { PCD_DIVMAPVAR, 1, "nvme" },
   { PCD_DIVWORLDVAR, 1, "nvwe" },
   { PCD_MODSCRIPTVAR, 1, "nvse" },
   { PCD_MODMAPVAR, 1, "nvme" },
   { PCD_MODWORLDVAR, 1, "nvwe" },
   { PCD_INCSCRIPTVAR, 1, "nvse" },
   { PCD_INCMAPVAR, 1, "nvme" },
   { PCD_INCWORLDVAR, 1, "nvwe" },
   { PCD_DECSCRIPTVAR, 1, "nvse" },
   { PCD_DECMAPVAR, 1, "nvme" },
   { PCD_DECWORLDVAR, 1, "nvwe" },
   { PCD_GOTO, 1, "nle" },
   { PCD_IFGOTO, 1, "nle" },
   { PCD_DROP, 0, "" },
   { PCD_DELAY, 0, "" },
   { PCD_DELAYDIRECT, 1, "ne" },
   { PCD_RANDOM, 0, "" },
   { PCD_RANDOMDIRECT, 2, "ne,ne" },
   { PCD_THINGCOUNT, 0, "" },
   { PCD_THINGCOUNTDIRECT, 2, "ne,ne" },
   { PCD_TAGWAIT, 0, "" },
   { PCD_TAGWAITDIRECT, 1, "ne" },
   { PCD_POLYWAIT, 0, "" },
   { PCD_POLYWAITDIRECT, 1, "ne" },
   { PCD_CHANGEFLOOR, 0, "" },
   { PCD_CHANGEFLOORDIRECT, 2, "ne,nse" },
   { PCD_CHANGECEILING, 0, "" },
   { PCD_CHANGECEILINGDIRECT, 2, "ne,nse" },
   { PCD_RESTART, 0, "" },
   { PCD_ANDLOGICAL, 0, "" },
   { PCD_ORLOGICAL, 0, "" },
   { PCD_ANDBITWISE, 0, "" },
   { PCD_ORBITWISE, 0, "" },
   { PCD_EORBITWISE, 0, "" },
   { PCD_NEGATELOGICAL, 0, "" },
   { PCD_LSHIFT, 0, "" },
   { PCD_RSHIFT, 0, "" },
   { PCD_UNARYMINUS, 0, "" },
   { PCD_IFNOTGOTO, 1, "nle" },
   { PCD_LINESIDE, 0, "" },
   { PCD_SCRIPTWAIT, 0, "" },
   { PCD_SCRIPTWAITDIRECT, 1, "ne" },
   { PCD_CLEARLINESPECIAL, 0, "" },
   { PCD_CASEGOTO, 2, "ne,nl" },
   { PCD_BEGINPRINT, 0, "" },
   { PCD_ENDPRINT, 0, "" },
   { PCD_PRINTSTRING, 0, "" },
   { PCD_PRINTNUMBER, 0, "" },
   { PCD_PRINTCHARACTER, 0, "" },
   { PCD_PLAYERCOUNT, 0, "" },
   { PCD_GAMETYPE, 0, "" },
   { PCD_GAMESKILL, 0, "" },
   { PCD_TIMER, 0, "" },
   { PCD_SECTORSOUND, 0, "" },
   { PCD_AMBIENTSOUND, 0, "" },
   { PCD_SOUNDSEQUENCE, 0, "" },
   { PCD_SETLINETEXTURE, 0, "" },
   { PCD_SETLINEBLOCKING, 0, "" },
   { PCD_SETLINESPECIAL, 0, "" },
   { PCD_THINGSOUND, 0, "" },
   { PCD_ENDPRINTBOLD, 0, "" },
   { PCD_ACTIVATORSOUND, 0, "" },
   { PCD_LOCALAMBIENTSOUND, 0, "" },
   { PCD_SETLINEMONSTERBLOCKING, 0, "" },
   { PCD_PLAYERBLUESKULL, 0, "" },
   { PCD_PLAYERREDSKULL, 0, "" },
   { PCD_PLAYERYELLOWSKULL, 0, "" },
   { PCD_PLAYERMASTERSKULL, 0, "" },
   { PCD_PLAYERBLUECARD, 0, "" },
   { PCD_PLAYERREDCARD, 0, "" },
   { PCD_PLAYERYELLOWCARD, 0, "" },
   { PCD_PLAYERMASTERCARD, 0, "" },
   { PCD_PLAYERBLACKSKULL, 0, "" },
   { PCD_PLAYERSILVERSKULL, 0, "" },
   { PCD_PLAYERGOLDSKULL, 0, "" },
   { PCD_PLAYERBLACKCARD, 0, "" },
   { PCD_PLAYERSILVERCARD, 0, "" },
   { PCD_ISNETWORKGAME, 0, "" },
   { PCD_PLAYERTEAM, 0, "" },
   { PCD_PLAYERHEALTH, 0, "" },
   { PCD_PLAYERARMORPOINTS, 0, "" },
   { PCD_PLAYERFRAGS, 0, "" },
   { PCD_PLAYEREXPERT, 0, "" },
   { PCD_BLUETEAMCOUNT, 0, "" },
   { PCD_REDTEAMCOUNT, 0, "" },
   { PCD_BLUETEAMSCORE, 0, "" },
   { PCD_REDTEAMSCORE, 0, "" },
   { PCD_ISONEFLAGCTF, 0, "" },
   { PCD_GETINVASIONWAVE, 0, "" },
   { PCD_GETINVASIONSTATE, 0, "" },
   { PCD_PRINTNAME, 0, "" },
   { PCD_MUSICCHANGE, 0, "" },
   { PCD_CONSOLECOMMANDDIRECT, 3, "nse,n,n" },
   { PCD_CONSOLECOMMAND, 0, "" },
   { PCD_SINGLEPLAYER, 0, "" },
   { PCD_FIXEDMUL, 0, "" },
   { PCD_FIXEDDIV, 0, "" },
   { PCD_SETGRAVITY, 0, "" },
   { PCD_SETGRAVITYDIRECT, 1, "ne" },
   { PCD_SETAIRCONTROL, 0, "" },
   { PCD_SETAIRCONTROLDIRECT, 1, "ne" },
   { PCD_CLEARINVENTORY, 0, "" },
   { PCD_GIVEINVENTORY, 0, "" },
   { PCD_GIVEINVENTORYDIRECT, 2, "nse,ne" },
   { PCD_TAKEINVENTORY, 0, "" },
   { PCD_TAKEINVENTORYDIRECT, 2, "nse,ne" },
   { PCD_CHECKINVENTORY, 0, "" },
   { PCD_CHECKINVENTORYDIRECT, 1, "nse" },
   { PCD_SPAWN, 0, "" },
   { PCD_SPAWNDIRECT, 6, "nse,ne,ne,ne,ne,ne" },
   { PCD_SPAWNSPOT, 0, "" },
   { PCD_SPAWNSPOTDIRECT, 4, "nse,ne,ne,ne" },
   { PCD_SETMUSIC, 0, "" },
   { PCD_SETMUSICDIRECT, 3, "nse,ne,ne" },
   { PCD_LOCALSETMUSIC, 0, "" },
   { PCD_LOCALSETMUSICDIRECT, 3, "nse,ne,ne" },
   { PCD_PRINTFIXED, 0, "" },
   { PCD_PRINTLOCALIZED, 0, "" },
   { PCD_MOREHUDMESSAGE, 0, "" },
   { PCD_OPTHUDMESSAGE, 0, "" },
   { PCD_ENDHUDMESSAGE, 0, "" },
   { PCD_ENDHUDMESSAGEBOLD, 0, "" },
   { PCD_SETSTYLE, 0, "" },
   { PCD_SETSTYLEDIRECT, 0, "" },
   { PCD_SETFONT, 0, "" },
   { PCD_SETFONTDIRECT, 1, "nse" },
   { PCD_PUSHBYTE, 1, "nse" },
   { PCD_LSPEC1DIRECTB, 2, "ne,ne" },
   { PCD_LSPEC2DIRECTB, 3, "ne,ne,ne" },
   { PCD_LSPEC3DIRECTB, 4, "ne,ne,ne,ne" },
   { PCD_LSPEC4DIRECTB, 5, "ne,ne,ne,ne,ne" },
   { PCD_LSPEC5DIRECTB, 6, "ne,ne,ne,ne,ne,ne" },
   { PCD_DELAYDIRECTB, 1, "ne" },
   { PCD_RANDOMDIRECTB, 2, "ne,ne" },
   // FIXME: This argument specification requests at least two arguments, but
   // the pushbytes should be able to take at least one argument, a zero.
   { PCD_PUSHBYTES, VARIABLE_ARGC, "ne,+nse" },
   { PCD_PUSH2BYTES, 2, "nse,nse" },
   { PCD_PUSH3BYTES, 3, "nse,nse,nse" },
   { PCD_PUSH4BYTES, 4, "nse,nse,nse,nse" },
   { PCD_PUSH5BYTES, 5, "nse,nse,nse,nse,nse" },
   { PCD_SETTHINGSPECIAL, 0, "" },
   { PCD_ASSIGNGLOBALVAR, 1, "nvge" },
   { PCD_PUSHGLOBALVAR, 1, "nvge" },
   { PCD_ADDGLOBALVAR, 1, "nvge" },
   { PCD_SUBGLOBALVAR, 1, "nvge" },
   { PCD_MULGLOBALVAR, 1, "nvge" },
   { PCD_DIVGLOBALVAR, 1, "nvge" },
   { PCD_MODGLOBALVAR, 1, "nvge" },
   { PCD_INCGLOBALVAR, 1, "nvge" },
   { PCD_DECGLOBALVAR, 1, "nvge" },
   { PCD_FADETO, 0, "" },
   { PCD_FADERANGE, 0, "" },
   { PCD_CANCELFADE, 0, "" },
   { PCD_PLAYMOVIE, 0, "" },
   { PCD_SETFLOORTRIGGER, 0, "" },
   { PCD_SETCEILINGTRIGGER, 0, "" },
   { PCD_GETACTORX, 0, "" },
   { PCD_GETACTORY, 0, "" },
   { PCD_GETACTORZ, 0, "" },
   { PCD_STARTTRANSLATION, 0, "" },
   { PCD_TRANSLATIONRANGE1, 0, "" },
   { PCD_TRANSLATIONRANGE2, 0, "" },
   { PCD_ENDTRANSLATION, 0, "" },
   { PCD_CALL, 1, "nefu" },
   { PCD_CALLDISCARD, 1, "nefu" },
   { PCD_RETURNVOID, 0, "" },
   { PCD_RETURNVAL, 0, "" },
   { PCD_PUSHMAPARRAY, 1, "name" },
   { PCD_ASSIGNMAPARRAY, 1, "name" },
   { PCD_ADDMAPARRAY, 1, "name" },
   { PCD_SUBMAPARRAY, 1, "name" },
   { PCD_MULMAPARRAY, 1, "name" },
   { PCD_DIVMAPARRAY, 1, "name" },
   { PCD_MODMAPARRAY, 1, "name" },
   { PCD_INCMAPARRAY, 1, "name" },
   { PCD_DECMAPARRAY, 1, "name" },
   { PCD_DUP, 0, "" },
   { PCD_SWAP, 0, "" },
   { PCD_WRITETOINI, 0, "" },
   { PCD_GETFROMINI, 0, "" },
   { PCD_SIN, 0, "" },
   { PCD_COS, 0, "" },
   { PCD_VECTORANGLE, 0, "" },
   { PCD_CHECKWEAPON, 0, "" },
   { PCD_SETWEAPON, 0, "" },
   { PCD_TAGSTRING, 0, "" },
   { PCD_PUSHWORLDARRAY, 1, "nawe" },
   { PCD_ASSIGNWORLDARRAY, 1, "nawe" },
   { PCD_ADDWORLDARRAY, 1, "nawe" },
   { PCD_SUBWORLDARRAY, 1, "nawe" },
   { PCD_MULWORLDARRAY, 1, "nawe" },
   { PCD_DIVWORLDARRAY, 1, "nawe" },
   { PCD_MODWORLDARRAY, 1, "nawe" },
   { PCD_INCWORLDARRAY, 1, "nawe" },
   { PCD_DECWORLDARRAY, 1, "nawe" },
   { PCD_PUSHGLOBALARRAY, 1, "nage" },
   { PCD_ASSIGNGLOBALARRAY, 1, "nage" },
   { PCD_ADDGLOBALARRAY, 1, "nage" },
   { PCD_SUBGLOBALARRAY, 1, "nage" },
   { PCD_MULGLOBALARRAY, 1, "nage" },
   { PCD_DIVGLOBALARRAY, 1, "nage" },
   { PCD_MODGLOBALARRAY, 1, "nage" },
   { PCD_INCGLOBALARRAY, 1, "nage" },
   { PCD_DECGLOBALARRAY, 1, "nage" },
   { PCD_SETMARINEWEAPON, 0, "" },
   { PCD_SETACTORPROPERTY, 0, "" },
   { PCD_GETACTORPROPERTY, 0, "" },
   { PCD_PLAYERNUMBER, 0, "" },
   { PCD_ACTIVATORTID, 0, "" },
   { PCD_SETMARINESPRITE, 0, "" },
   { PCD_GETSCREENWIDTH, 0, "" },
   { PCD_GETSCREENHEIGHT, 0, "" },
   { PCD_THINGPROJECTILE2, 0, "" },
   { PCD_STRLEN, 0, "" },
   { PCD_SETHUDSIZE, 0, "" },
   { PCD_GETCVAR, 0, "" },
   { PCD_CASEGOTOSORTED, VARIABLE_ARGC, "ne,+nle" },
   { PCD_SETRESULTVALUE, 0, "" },
   { PCD_GETLINEROWOFFSET, 0, "" },
   { PCD_GETACTORFLOORZ, 0, "" },
   { PCD_GETACTORANGLE, 0, "" },
   { PCD_GETSECTORFLOORZ, 0, "" },
   { PCD_GETSECTORCEILINGZ, 0, "" },
   { PCD_LSPEC5RESULT, 1, "ne" },
   { PCD_GETSIGILPIECES, 0, "" },
   { PCD_GETLEVELINFO, 0, "" },
   { PCD_CHANGESKY, 0, "" },
   { PCD_PLAYERINGAME, 0, "" },
   { PCD_PLAYERISBOT, 0, "" },
   { PCD_SETCAMERATOTEXTURE, 0, "" },
   { PCD_ENDLOG, 0, "" },
   { PCD_GETAMMOCAPACITY, 0, "" },
   { PCD_SETAMMOCAPACITY, 0, "" },
   { PCD_PRINTMAPCHARARRAY, 0, "" },
   { PCD_PRINTWORLDCHARARRAY, 0, "" },
   { PCD_PRINTGLOBALCHARARRAY, 0, "" },
   { PCD_SETACTORANGLE, 0, "" },
   { PCD_GRAPINPUT, 0, "" },
   { PCD_SETMOUSEPOINTER, 0, "" },
   { PCD_MOVEMOUSEPOINTER, 0, "" },
   { PCD_SPAWNPROJECTILE, 0, "" },
   { PCD_GETSECTORLIGHTLEVEL, 0, "" },
   { PCD_GETACTORCEILINGZ, 0, "" },
   { PCD_SETACTORPOSITION, 0, "" },
   { PCD_CLEARACTORINVENTORY, 0, "" },
   { PCD_GIVEACTORINVENTORY, 0, "" },
   { PCD_TAKEACTORINVENTORY, 0, "" },
   { PCD_CHECKACTORINVENTORY, 0, "" },
   { PCD_THINGCOUNTNAME, 0, "" },
   { PCD_SPAWNSPOTFACING, 0, "" },
   { PCD_PLAYERCLASS, 0, "" },
   { PCD_ANDSCRIPTVAR, 1, "nvse" },
   { PCD_ANDMAPVAR, 1, "nvme" },
   { PCD_ANDWORLDVAR, 1, "nvwe" },
   { PCD_ANDGLOBALVAR, 1, "nvge" },
   { PCD_ANDMAPARRAY, 1, "name" },
   { PCD_ANDWORLDARRAY, 1, "nawe" },
   { PCD_ANDGLOBALARRAY, 1, "nage" },
   { PCD_EORSCRIPTVAR, 1, "nvse" },
   { PCD_EORMAPVAR, 1, "nvme" },
   { PCD_EORWORLDVAR, 1, "nvwe" },
   { PCD_EORGLOBALVAR, 1, "nvge" },
   { PCD_EORMAPARRAY, 1, "name" },
   { PCD_EORWORLDARRAY, 1, "nawe" },
   { PCD_EORGLOBALARRAY, 1, "nage" },
   { PCD_ORSCRIPTVAR, 1, "nvse" },
   { PCD_ORMAPVAR, 1, "nvme" },
   { PCD_ORWORLDVAR, 1, "nvwe" },
   { PCD_ORGLOBALVAR, 1, "nvge" },
   { PCD_ORMAPARRAY, 1, "name" },
   { PCD_ORWORLDARRAY, 1, "nawe" },
   { PCD_ORGLOBALARRAY, 1, "nage" },
   { PCD_LSSCRIPTVAR, 1, "nvse" },
   { PCD_LSMAPVAR, 1, "nvme" },
   { PCD_LSWORLDVAR, 1, "nvwe" },
   { PCD_LSGLOBALVAR, 1, "nvge" },
   { PCD_LSMAPARRAY, 1, "name" },
   { PCD_LSWORLDARRAY, 1, "nawe" },
   { PCD_LSGLOBALARRAY, 1, "nage" },
   { PCD_RSSCRIPTVAR, 1, "nvse" },
   { PCD_RSMAPVAR, 1, "nvme" },
   { PCD_RSWORLDVAR, 1, "nvwe" },
   { PCD_RSGLOBALVAR, 1, "nvge" },
   { PCD_RSMAPARRAY, 1, "name" },
   { PCD_RSWORLDARRAY, 1, "nawe" },
   { PCD_RSGLOBALARRAY, 1, "nage" },
   { PCD_GETPLAYERINFO, 0, "" },
   { PCD_CHANGELEVEL, 0, "" },
   { PCD_SECTORDAMAGE, 0, "" },
   { PCD_REPLACETEXTURES, 0, "" },
   { PCD_NEGATEBINARY, 0, "" },
   { PCD_GETACTORPITCH, 0, "" },
   { PCD_SETACTORPITCH, 0, "" },
   { PCD_PRINTBIND, 0, "" },
   { PCD_SETACTORSTATE, 0, "" },
   { PCD_THINGDAMAGE2, 0, "" },
   { PCD_USEINVENTORY, 0, "" },
   { PCD_USEACTORINVENTORY, 0, "" },
   { PCD_CHECKACTORCEILINGTEXTURE, 0, "" },
   { PCD_CHECKACTORFLOORTEXTURE, 0, "" },
   { PCD_GETACTORLIGHTLEVEL, 0, "" },
   { PCD_SETMUGSHOTSTATE, 0, "" },
   { PCD_THINGCOUNTSECTOR, 0, "" },
   { PCD_THINGCOUNTNAMESECTOR, 0, "" },
   { PCD_CHECKPLAYERCAMERA, 0, "" },
   { PCD_MORPHACTOR, 0, "" },
   { PCD_UNMORPHACTOR, 0, "" },
   { PCD_GETPLAYERINPUT, 0, "" },
   { PCD_CLASSIFYACTOR, 0, "" },
   { PCD_PRINTBINARY, 0, "" },
   { PCD_PRINTHEX, 0, "" },
   { PCD_CALLFUNC, 2, "ne,nfee" },
   { PCD_SAVESTRING, 0, "" },
   { PCD_PRINTMAPCHRANGE, 0, "" },
   { PCD_PRINTWORLDCHRANGE, 0, "" },
   { PCD_PRINTGLOBALCHRANGE, 0, "" },
   { PCD_STRCPYTOMAPCHRANGE, 0, "" },
   { PCD_STRCPYTOWORLDCHRANGE, 0, "" },
   { PCD_STRCPYTOGLOBALCHRANGE, 0, "" },
   { PCD_PUSHFUNCTION, 1, "nefu" },
   { PCD_CALLSTACK, 0, "" },
   { PCD_SCRIPTWAITNAMED, 0, "" },
   { PCD_TRANSLATIONRANGE3, 0, "" },
   { PCD_GOTOSTACK, 0, "" },
   { PCD_ASSIGNSCRIPTARRAY, 1, "nase" },
   { PCD_PUSHSCRIPTARRAY, 1, "nase" },
   { PCD_ADDSCRIPTARRAY, 1, "nase" },
   { PCD_SUBSCRIPTARRAY, 1, "nase" },
   { PCD_MULSCRIPTARRAY, 1, "nase" },
   { PCD_DIVSCRIPTARRAY, 1, "nase" },
   { PCD_MODSCRIPTARRAY, 1, "nase" },
   { PCD_INCSCRIPTARRAY, 1, "nase" },
   { PCD_DECSCRIPTARRAY, 1, "nase" },
   { PCD_ANDSCRIPTARRAY, 1, "nase" },
   { PCD_EORSCRIPTARRAY, 1, "nase" },
   { PCD_ORSCRIPTARRAY, 1, "nase" },
   { PCD_LSSCRIPTARRAY, 1, "nase" },
   { PCD_RSSCRIPTARRAY, 1, "nase" },
   { PCD_PRINTSCRIPTCHARARRAY, 0, "" },
   { PCD_PRINTSCRIPTCHRANGE, 0, "" },
   { PCD_STRCPYTOSCRIPTCHRANGE, 0, "" },
   { PCD_LSPEC5EX, 1, "ne" },
   { PCD_LSPEC5EXRESULT, 1, "ne" },
   { PCD_TRANSLATIONRANGE4, 0, "" },
   { PCD_TRANSLATIONRANGE5, 0, "" },
};

struct pcode* c_get_pcode_info( int code ) {
   STATIC_ASSERT( PCD_TOTAL == 385 );
   if ( code < ARRAY_SIZE( pcode_info ) ) {
      return ( pcode_info + code );
   }
   return NULL;
}

static struct direct_pcode g_direct_pcode_table[] = {
   { PCD_LSPEC1, PCD_LSPEC1DIRECT, 1 },
   { PCD_LSPEC2, PCD_LSPEC2DIRECT, 2 },
   { PCD_LSPEC3, PCD_LSPEC3DIRECT, 3 },
   { PCD_LSPEC4, PCD_LSPEC4DIRECT, 4 },
   { PCD_LSPEC5, PCD_LSPEC5DIRECT, 5 },
   { PCD_DELAY, PCD_DELAYDIRECT, 1 },
   { PCD_RANDOM, PCD_RANDOMDIRECT, 2 },
   { PCD_THINGCOUNT, PCD_THINGCOUNTDIRECT, 2 },
   { PCD_TAGWAIT, PCD_TAGWAITDIRECT, 1 },
   { PCD_POLYWAIT, PCD_POLYWAITDIRECT, 1 },
   { PCD_CHANGEFLOOR, PCD_CHANGEFLOORDIRECT, 2 },
   { PCD_CHANGECEILING, PCD_CHANGECEILINGDIRECT, 2 },
   { PCD_SCRIPTWAIT, PCD_SCRIPTWAITDIRECT, 1 },
   { PCD_CONSOLECOMMAND, PCD_CONSOLECOMMANDDIRECT, 3 },
   { PCD_SETGRAVITY, PCD_SETGRAVITYDIRECT, 1 },
   { PCD_SETAIRCONTROL, PCD_SETAIRCONTROLDIRECT, 1 },
   { PCD_GIVEINVENTORY, PCD_GIVEINVENTORYDIRECT, 2 },
   { PCD_TAKEINVENTORY, PCD_TAKEINVENTORYDIRECT, 2 },
   { PCD_CHECKINVENTORY, PCD_CHECKINVENTORYDIRECT, 1 },
   { PCD_SPAWN, PCD_SPAWNDIRECT, 6 },
   { PCD_SPAWNSPOT, PCD_SPAWNSPOTDIRECT, 4 },
   { PCD_SETMUSIC, PCD_SETMUSICDIRECT, 3 },
   { PCD_LOCALSETMUSIC, PCD_LOCALSETMUSICDIRECT, 3 },
   { PCD_SETFONT, PCD_SETFONTDIRECT, 1 }
};

const struct direct_pcode* c_get_direct_pcode( int code ) { 
   for ( int i = 0; i < ARRAY_SIZE( g_direct_pcode_table ); ++i ) {
      if ( g_direct_pcode_table[ i ].code == code ) {
         return &g_direct_pcode_table[ i ];
      }
   }
   return NULL;
}
