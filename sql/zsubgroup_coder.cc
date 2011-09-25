/* Copyright (c) 2011, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#include "zgroups.h"


#ifdef HAVE_UGID


enum_append_status Subgroup_coder::append(
  Appender *appender, const Cached_subgroup *cs,
  rpl_binlog_pos offset_after_last_statement,
  bool group_commit, uint32 owner_type)
{
  DBUG_ENTER("Group_log::Read_state::write");
  uchar buf[FULL_SUBGROUP_SIZE];
  uchar *p= buf;
  // write type code
  *p= SPECIAL_TYPE, p++;
  *p= FULL_SUBGROUP, p++;
  // write group information
  *p= cs->type, p++;
  int4store(p, cs->sidno); p+= 4;
  int8store(p, cs->gno); p+= 8;
  int8store(p, binlog_no); p+= 8;
  int8store(p, binlog_pos); p+= 8;
  int8store(p, cs->binlog_length); p+= 8;
  int8store(p, offset_after_last_statement); p+= 8;
  int4store(p, owner_type); p+= 4;
  *p= cs->group_end ? 1 : 0, p++;
  *p= group_commit ? 1 : 0, p++;
  // append
  PROPAGATE_APPEND_STATUS(appender->append(buf, p - buf));
  // update state
  lgid++;
  binlog_pos+= cs->binlog_length;
  DBUG_RETURN(APPEND_OK);
}


enum_read_status Subgroup_coder::read(Reader *reader, Subgroup *out)
{
  DBUG_ENTER("Group_log::Read_state::read");
  uchar buf[FULL_SUBGROUP_SIZE];
  my_off_t saved_pos;
  if (reader->tell(&saved_pos) != RETURN_STATUS_OK)
    DBUG_RETURN(READ_ERROR);
  READER_READ(reader, saved_pos, 2, buf);
  READER_CHECK_FORMAT(reader, saved_pos,
                      buf[0] == SPECIAL_TYPE &&
                      (buf[1] == FULL_SUBGROUP ||
                       (buf[1] >= MIN_IGNORABLE_TYPE && (buf[1] & 1) == 1)));
  READER_READ(reader, saved_pos, FULL_SUBGROUP_SIZE, buf);
#define UNPACK(FIELD, N) out->FIELD= sint ## N ## korr(p), p += N
  uchar *p= buf;
  UNPACK(sidno, 4);
  UNPACK(gno, 8);
  UNPACK(binlog_no, 8);
  UNPACK(binlog_pos, 8);
  UNPACK(binlog_length, 8);
  UNPACK(binlog_offset_after_last_statement, 8);
  out->group_end= *p == 1 ? true : false, p++;
  out->group_commit= *p == 1 ? true : false, p++;
  out->lgid= ++lgid;
  DBUG_RETURN(READ_OK);
}


#endif