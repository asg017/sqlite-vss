.bail on
.load ../../../sqlite-vector/build_release/vector0
.load ../../build_release/vss0
.timer on
.mode box
.header on

attach "simonwillisonblog.db" as simonwillisonblog;

create table blog_entry as 
  select 
    entries.*, 
    embeddings.embedding as body_embedding
  from simonwillisonblog.blog_entry as entries
  left join simonwillisonblog.blog_entry_embeddings as embeddings 
    on embeddings.id = entries.id
  where body_embedding is not null;

create virtual table vss_blog_entry using vss0(
  body_embedding(1536)
);

insert into vss_blog_entry(rowid, body_embedding)
  select id, body_embedding from blog_entry;