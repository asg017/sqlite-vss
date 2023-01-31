import { Database } from "https://deno.land/x/sqlite3@0.7.2/mod.ts";
import wtf from 'npm:wtf_wikipedia';

const db = new Database(Deno.args[0]);

interface Row {
  rowid: number;
  article: string;
  title: string;
};
for(const r of db.prepare("select rowid, title, article from pages where title not like '%:%'")) {
  const row:Row = r as Row;
  const doc = wtf(row.article);
  for(const section of doc.sections()) {
    if(section.title() === 'Plot') {
      for(const [i, sentence] of (section.sentences() as any[]).entries()) {
        db.exec(`
        insert into plot_sentences(page, sentence, contents)
        values (?, ?, ?)
        `, row.rowid, i, sentence.text())
      }
      
    }
  }
}


db.close();