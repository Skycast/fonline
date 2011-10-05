#ifndef __ACCESS__
#define __ACCESS__

#define ACCESS_CLIENT                BIN8( 00000001 )
#define ACCESS_TESTER                BIN8( 00000010 )
#define ACCESS_MODER                 BIN8( 00000100 )
#define ACCESS_ADMIN                 BIN8( 00001000 )
#define ACCESS_IMPLEMENTOR           BIN8( 00010000 )
#define ACCESS_ALL                   BIN8( 11111111 )

#ifdef DEV_VESRION
# define ACCESS_DEFAULT              ACCESS_TESTER
#else
# define ACCESS_DEFAULT              ACCESS_CLIENT
#endif

#define CMD_EXIT                     ( 1 )
#define CMD_MYINFO                   ( 2 )
#define CMD_GAMEINFO                 ( 3 )
#define CMD_CRITID                   ( 4 )
#define CMD_MOVECRIT                 ( 5 )
#define CMD_KILLCRIT                 ( 6 )
#define CMD_DISCONCRIT               ( 7 )
#define CMD_TOGLOBAL                 ( 8 )
#define CMD_RESPAWN                  ( 9 )
#define CMD_PARAM                    ( 10 )
#define CMD_GETACCESS                ( 11 )
#define CMD_ADDITEM                  ( 12 )
#define CMD_ADDITEM_SELF             ( 13 )
#define CMD_ADDNPC                   ( 14 )
#define CMD_ADDLOCATION              ( 15 )
#define CMD_RELOADSCRIPTS            ( 16 )
#define CMD_LOADSCRIPT               ( 17 )
#define CMD_RELOAD_CLIENT_SCRIPTS    ( 18 )
#define CMD_RUNSCRIPT                ( 19 )
#define CMD_RELOADLOCATIONS          ( 20 )
#define CMD_LOADLOCATION             ( 21 )
#define CMD_RELOADMAPS               ( 22 )
#define CMD_LOADMAP                  ( 23 )
#define CMD_REGENMAP                 ( 24 )
#define CMD_RELOADDIALOGS            ( 25 )
#define CMD_LOADDIALOG               ( 26 )
#define CMD_RELOADTEXTS              ( 27 )
#define CMD_RELOADAI                 ( 28 )
#define CMD_CHECKVAR                 ( 29 )
#define CMD_SETVAR                   ( 30 )
#define CMD_SETTIME                  ( 31 )
#define CMD_BAN                      ( 32 )
#define CMD_DELETE_ACCOUNT           ( 33 )
#define CMD_CHANGE_PASSWORD          ( 34 )
#define CMD_DROP_UID                 ( 35 )
#define CMD_LOG                      ( 36 )

// non-public
#define CMD_CRASH                    ( 142 )

/*
   // Deprecated, left as reference

   #define CMD_EXIT_ACCESS                     ( ACCESS_ALL )
   #define CMD_MYINFO_ACCESS                   ( ACCESS_ALL )
   #define CMD_GAMEINFO_ACCESS                 ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_CRITID_ACCESS                   ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_MOVECRIT_ACCESS                 ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_KILLCRIT_ACCESS                 ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_DISCONCRIT_ACCESS               ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_TOGLOBAL_ACCESS                 ( ACCESS_TESTER | ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RESPAWN_ACCESS                  ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_PARAM_ACCESS                    ( ACCESS_TESTER | ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_GETACCESS_ACCESS                ( ACCESS_ALL )
   #define CMD_CRASH_ACCESS                    ( ACCESS_IMPLEMENTOR )
   #define CMD_ADDITEM_ACCESS                  ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_ADDITEM_SELF_ACCESS             ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_ADDNPC_ACCESS                   ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_ADDLOCATION_ACCESS              ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RELOADSCRIPTS_ACCESS            ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_LOADSCRIPT_ACCESS               ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RELOAD_CLIENT_SCRIPTS_ACCESS    ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RUNSCRIPT_ACCESS                ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RELOADLOCATIONS_ACCESS          ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_LOADLOCATION_ACCESS             ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RELOADMAPS_ACCESS               ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_LOADMAP_ACCESS                  ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_REGENMAP_ACCESS                 ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RELOADDIALOGS_ACCESS            ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_LOADDIALOG_ACCESS               ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RELOADTEXTS_ACCESS              ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_RELOADAI_ACCESS                 ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_CHECKVAR_ACCESS                 ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_SETVAR_ACCESS                   ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_SETTIME_ACCESS                  ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_BAN_ACCESS                      ( ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_DELETE_ACCOUNT_ACCESS           ( ACCESS_ALL )
   #define CMD_CHANGE_PASSWORD_ACCESS          ( ACCESS_ALL )
   #define CMD_DROP_UID_ACCESS                 ( ACCESS_TESTER | ACCESS_MODER | ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
   #define CMD_LOG_ACCESS                      ( ACCESS_ADMIN | ACCESS_IMPLEMENTOR )
 */

#endif // __ACCESS__
