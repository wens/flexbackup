


 MMTT((11LL)) 							      MMTT((11LL))




 NNAAMMEE
      mt - control magnetic tape drive operation

 SSYYNNOOPPSSIISS
      mmtt [-V] [-f device] [--file=device] [--version] operation [count]

 DDEESSCCRRIIPPTTIIOONN
      This manual page documents the GNU version of mmtt.  mmtt performs the
      given _o_p_e_r_a_t_i_o_n, which must be one of the tape operations listed
      below, on a tape drive.

      The default tape device to operate on is taken from the file
      _/_u_s_r_/_i_n_c_l_u_d_e_/_s_y_s_/_m_t_i_o_._h when mmtt is compiled.  It can be overridden by
      giving a device file name in the environment variable TTAAPPEE or by a
      command line option (see below), which also overrides the environment
      variable.

      The device must be either a character special file or a remote tape
      drive.  To use a tape drive on another machine as the archive, use a
      filename that starts with `HOSTNAME:'.  The hostname can be preceded
      by a username and an `@' to access the remote tape drive as that user,
      if you have permission to do so (typically an entry in that user's
      `~/.rhosts' file).

      The available operations are listed below.  Unique abbreviations are
      accepted.  Not all operations are available on all systems, or work on
      all types of tape drives.  Some operations optionally take a repeat
      count, which can be given after the operation name and defaults to 1.

      eof, weof
	   Write _c_o_u_n_t EOF marks at current position.

      fsf  Forward space _c_o_u_n_t files.  The tape is positioned on the first
	   block of the next file.

      bsf  Backward space _c_o_u_n_t files.	The tape is positioned on the first
	   block of the next file.

      fsr  Forward space _c_o_u_n_t records.

      bsr  Backward space _c_o_u_n_t records.

      bsfm Backward space _c_o_u_n_t file marks.  The tape is positioned on the
	   beginning-of-the-tape side of the file mark.

      fsfm Forward space _c_o_u_n_t file marks.  The tape is positioned on the
	   beginning-of-the-tape side of the file mark.

      asf  Absolute space to file number _c_o_u_n_t.  Equivalent to rewind
	   followed by fsf _c_o_u_n_t.




				    - 1 -	    Formatted:	June 5, 1996






 MMTT((11LL)) 							      MMTT((11LL))




      seek Seek to block number _c_o_u_n_t.

      eom  Space to the end of the recorded media on the tape (for appending
	   files onto tapes).

      rewind
	   Rewind the tape.

      offline, rewoffl
	   Rewind the tape and, if applicable, unload the tape.

      status
	   Print status information about the tape unit.

      retension
	   Rewind the tape, then wind it to the end of the reel, then rewind
	   it again.

      erase
	   Erase the tape.

      mmtt exits with a status of 0 if the operation succeeded, 1 if the
      operation or device name given was invalid, or 2 if the operation
      failed.

    OOPPTTIIOONNSS
      _-_f_, _-_-_f_i_l_e_=_d_e_v_i_c_e
	   Use _d_e_v_i_c_e as the file name of the tape drive to operate on.  To
	   use a tape drive on another machine, use a filename that starts
	   with `HOSTNAME:'.  The hostname can be preceded by a username and
	   an `@' to access the remote tape drive as that user, if you have
	   permission to do so (typically an entry in that user's
	   `~/.rhosts' file).

      _-_V_, _-_-_v_e_r_s_i_o_n
	   Print the version number of mmtt.


















				    - 2 -	    Formatted:	June 5, 1996



