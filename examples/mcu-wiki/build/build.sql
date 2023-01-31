.bail on
.load ./xml0

.timer on

create table pages(
  id text primary key,
  title text,
  updated_at datetime,
  article text
);

CREATE TABLE plot_sentences(
  page integer, 
  sentence integer,
  contents text, 
  contents_embedding blob,
  foreign key (page) references pages(rowid)
);
insert into pages
  select 
    xml_extract(node, './/mediawiki:id/text()') as id,
    xml_extract(node, './/mediawiki:title/text()') as title,
    
    xml_extract(node, './/mediawiki:timestamp/text()') as updated_at,
    xml_extract(node, './/mediawiki:text/text()') as article
  from xml_each(
    readfile('marvelcinematicuniverse_pages_current.xml'),
    '//mediawiki:page',
    json_object(
      'mediawiki', 'http://www.mediawiki.org/xml/export-0.11/'
    )
  );
