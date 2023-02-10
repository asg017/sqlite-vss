.load ./lines0

create table articles(
  headline text,
  headline_embedding fvector,
  description text,
  description_embedding fvector,
  link text,
  category text,
  authors text,
  date
);

insert into articles(headline, description, link, category, authors, date)
  select
    line ->> '$.headline'           as headline,
    line ->> '$.short_description'  as description,
    line ->> '$.link'               as link,
    line ->> '$.category'           as category,
    line ->> '$.authors'            as authors,
    line ->> '$.date'               as date
  from lines_read('News_Category_Dataset_v3.json');