.bail on
.timer on

.load ./vss0
.load ./vector0

create virtual table vss_plot_sentences using vss_index(
  contents_embedding(384) with "Flat,IDMap2"
);

insert into vss_plot_sentences(rowid, contents_embedding)
  select 
    rowid,
    vector_from_blob(contents_embedding)
  from plot_sentences;

select count(*) from vss_plot_sentences_index;