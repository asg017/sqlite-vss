.bail on
.load ./vector0
.load ../../build/vss0

.timer on
.mode box
.header on

create virtual table vss_articles using vss0(
  headline_embedding(384),
  description_embedding(384),
);

-- 8s
insert into vss_articles(rowid, headline_embedding, description_embedding)
  select 
    rowid,
    headline_embedding,
    description_embedding
  from articles;


create virtual table vss_ivf_articles using vss0(
  headline_embedding(384) factory="IVF4096,Flat,IDMap2",
  description_embedding(384) factory="IVF4096,Flat,IDMap2"
);

-- 44m16s !
insert into vss_ivf_articles(operation, headline_embedding, description_embedding)
  select 
    'training',
    headline_embedding,
    description_embedding
  from articles;

-- 4m31s
insert into vss_ivf_articles(rowid, headline_embedding, description_embedding)
  select 
    rowid,
    headline_embedding,
    description_embedding
  from articles;
