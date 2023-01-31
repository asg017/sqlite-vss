.bail on
.load ./vector0
.load ../../build/vss0

.timer on
.mode box
.header on

create virtual table vss_articles using vss_index(
  headline_embedding(384) with "Flat,IDMap2"
);

insert into vss_articles(rowid, headline_embedding)
  select 
    rowid,
    vector_from_blob(headline_embedding)
  from articles;


-- "IVF100,PQ64"
create virtual table vss_articlesx using vss_index(
  headline_embedding(384) with "IVF4096,Flat,IDMap2"
);

insert into vss_articlesx(operation, headline_embedding)
  select 
    'training',
    vector_from_blob(headline_embedding)
  from articles;

insert into vss_articlesx(rowid, headline_embedding)
  select 
    rowid,
    vector_from_blob(headline_embedding)
  from articles;
