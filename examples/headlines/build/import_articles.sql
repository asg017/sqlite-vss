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
    json_extract(line, '$.headline')           as headline,
    json_extract(line, '$.short_description')  as description,
    json_extract(line, '$.link')               as link,
    json_extract(line, '$.category')           as category,
    json_extract(line, '$.authors')            as authors,
    json_extract(line, '$.date')               as date
  from lines_read('News_Category_Dataset_v3.json');