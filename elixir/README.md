# SqliteVss

**TODO: Add description**

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `sqlite_vss` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:sqlite_vss, "~> 0.0.0"}
  ]
end
```

Once installed, change your `config/config.exs` to pick your sqlite_vss version of choice:

`config :sqlite_vss, version: "0.0.0"`

Now you can install sqlite_vss by running:

`$ mix sqlite_vss.install`

Documentation can be generated with [ExDoc](https://github.com/elixir-lang/ex_doc)
and published on [HexDocs](https://hexdocs.pm). Once published, the docs can
be found at <https://hexdocs.pm/sqlite_vss>.

Add the following to your application 
```
config :my_app, MyApp.Repo,
  after_connect: {SqliteVss.Extension, :enable, []}
```

Create a new migration for adding the embedding columns to the table with the data we want to search.
```
defmodule MyApp.Repo.Migrations.AddEmbeddingColumnsToPosts do
  use Ecto.Migration

  import Ecto.Query

  alias MyApp.Repo

  def change do
    create table("posts_to_migrate") do
      add :company, :string
      add :description, :string
      add :title, :string
      add :summary, :string

      timestamps(type: :utc_datetime)
    end

    flush()

    current_posts =
      Repo.all(
        from(p in "posts",
          select: [
            company: p.company,
            description: p.description,
            title: p.title,
            summary: p.summary,
            inserted_at: p.inserted_at,
            updated_at: p.updated_at
          ]
        )
      )

    current_posts
    |> Enum.chunk_every(500)
    |> Enum.each(fn chunk ->
      Repo.insert_all(
        "posts_to_migrate",
        chunk
      )
    end)

    drop table("posts")

    # add embedding columns
    create table("posts") do
      add :company, :string
      add :description, :string
      add :title, :string
      add :summary, :string
      add :title_embedding, :blob
      add :summary_embedding, :blob
      add :description_embedding, :blob

      timestamps(type: :utc_datetime)
    end

    flush()

    current_posts_to_migrate =
      Repo.all(
        from(p in "posts_to_migrate",
          select: [
            company: p.company,
            description: p.description,
            title: p.title,
            summary: p.summary,
            inserted_at: p.inserted_at,
            updated_at: p.updated_at
          ]
        )
      )

    current_posts_to_migrate
    |> Enum.chunk_every(500)
    |> Enum.each(fn chunk ->
      Repo.insert_all(
        "posts",
        chunk
      )
    end)

    drop table("posts_to_migrate")
  end
end
```

Create a new migration for the virtual table.
```
defmodule MyApp.Repo.Migrations.CreateVssPosts do
  use Ecto.Migration

  alias MyApp.Repo

  def up do
    Ecto.Adapters.SQL.query!(
      Repo,
      "CREATE VIRTUAL TABLE vss_posts USING vss0(title_embedding(384), summary_embedding(384), description_embedding(384));"
    )
  end

  def down do
    Ecto.Adapters.SQL.query!(Repo, "DROP table vss_posts;")
  end
end
```

Create a `ShadowPost` or shadow model to interact with the virtual table.
```
defmodule MyApp.Jobs.ShadowPost do
  use Ecto.Schema

  import Ecto.Changeset

  @primary_key {:rowid, :integer, []}

  schema "vss_posts" do
    field :title_embedding, :binary
    field :description_embedding, :binary
    field :summary_embedding, :binary
  end

  @doc false
  def changeset(post, attrs) do
    post
    |> cast(attrs, [
      :title_embedding,
      :summary_embedding,
      :description_embedding
    ])
    |> validate_required([
      :title_embedding,
      :summary_embedding,
      :description_embedding
    ])
  end
end
```

Create your own embedder with the following or get the embeddings from OpenAI.
```
defmodule MyApp.Embedder.SentenceTransformer do
  @moduledoc """
  Client which reaches out to Sentence Transformer for text embedding.
  """

  @behaviour MyApp.Embedder.SentenceTransformer.ClientBehaviour

  @doc """
  Builds the serving for the all-MiniLM-L6-v2 model.
  """
  @spec build_serving() :: Nx.Serving.t()
  def build_serving() do
    batch_size = 4
    defn_options = [compiler: EXLA]

    Nx.Serving.new(
      fn ->
        model_name = "sentence-transformers/all-MiniLM-L6-v2"
        {:ok, model_info} = Bumblebee.load_model({:hf, model_name})

        {_init_fun, predict_fun} = Axon.build(model_info.model)

        inputs_template = %{
          "attention_mask" => Nx.template({4, 128}, :u32),
          "input_ids" => Nx.template({4, 128}, :u32),
          "token_type_ids" => Nx.template({4, 128}, :u32)
        }

        template_args = [Nx.to_template(model_info.params), inputs_template]

        predict_fun = Nx.Defn.compile(predict_fun, template_args, defn_options)

        fn incoming_inputs ->
          inputs = Nx.Batch.pad(incoming_inputs, batch_size - incoming_inputs.size)
          predict_fun.(model_info.params, inputs)
        end
      end,
      batch_size: batch_size
    )
  end

  @doc """
  Generates the embedding for a given text.  Support up to 4 texts at a time.
  """
  @spec to_embedding!([String.t()]) :: [binary()]
  def to_embedding!(texts) do
    model_name = "sentence-transformers/all-MiniLM-L6-v2"

    {:ok, model_info} = Bumblebee.load_model({:hf, model_name})
    {:ok, tokenizer} = Bumblebee.load_tokenizer({:hf, model_name})

    texts = Enum.filter(texts, & &1 != "" && &1 != nil)

    text_inputs = 
      for text <- texts do
        Bumblebee.apply_tokenizer(tokenizer, [text])
      end

    text_batch = Nx.Batch.concatenate(text_inputs)

    text_results = Nx.Serving.batched_run(MyApp.Embedder.SentenceTransformer, text_batch)

    for {text_input, i} <- Enum.with_index(text_inputs) do
      text_result_to_embedding(text_results, Enum.at(text_inputs, i), i)
    end
  end

  defp text_result_to_embedding(text_results, text_input, i) do
    text_attention_mask = text_input["attention_mask"]
    text_input_mask_expanded = Nx.new_axis(text_attention_mask, -1)

    text_results.hidden_state[i]
    |> Nx.multiply(text_input_mask_expanded)
    |> Nx.sum(axes: [1])
    |> Nx.divide(Nx.sum(text_input_mask_expanded, axes: [1]))
    |> Scholar.Preprocessing.normalize(norm: :euclidean)
    |> Nx.to_binary()
  end
end
```

Wrap any calls to the above with something like Oban.
```
defmodule MyApp.Embed do
  @module_doc """
  Job which embeds text using the SentenceTransformer model.
  """

  use Oban.Worker, max_attempts: 3, queue: :embed, unique: [fields: [:args], keys: [:job_url]]

  import Ecto.Query

  alias MyApp.Jobs.Post
  alias MyApp.Jobs.ShadowPost
  alias MyApp.Repo

  def get_embedder_impl() do
    Application.get_env(:soft_jobs, MyApp.Embedder.SentenceTransformer)[:client]
  end

  @doc """
  Oban worker which embeds text using SentenceTransformer model.

  The from columns must be populated before this job is run.
  Currently supports up to 4 job urls at a time.
  """
  @impl Oban.Worker
  def perform(%Oban.Job{args: %{"job_urls" => job_urls}}) do
    query = from p in Post, where: p.job_url in ^job_urls
    posts = Repo.all(query)

    shadow_posts_query = from p in ShadowPost, where: p.rowid in ^Enum.map(posts, & &1.id)
    shadow_posts = Repo.all(shadow_posts_query)

    post_titles = Enum.map(posts, & &1.title)
    post_summaries = Enum.map(posts, & &1.summary)
    post_descriptions = Enum.map(posts, & &1.description)

    title_embeddings = get_embedder_impl().to_embedding!(post_titles)
    summary_embeddings = get_embedder_impl().to_embedding!(post_summaries)
    description_embeddings = get_embedder_impl().to_embedding!(post_descriptions)

    for {post, i} <- Enum.with_index(posts) do
      title_embedding = Enum.at(title_embeddings, i)
      summary_embedding = Enum.at(summary_embeddings, i)
      description_embedding = Enum.at(description_embeddings, i)

      {:ok, post} = update_post(post, title_embedding, summary_embedding, description_embedding)

      update_shadow_post(
        post,
        title_embedding,
        summary_embedding,
        description_embedding
      )
    end

    :ok
  end

  defp update_post(post, title_embedding, summary_embedding, description_embedding) do
    post
    |> Post.changeset(%{
      title_embedding: title_embedding,
      summary_embedding: summary_embedding,
      description_embedding: description_embedding
    })
    |> Repo.update()
  end

  defp update_shadow_post(post, title_embedding, summary_embedding, description_embedding) do
    if shadow_post = Repo.get_by(ShadowPost, rowid: post.id) do
      Repo.delete!(shadow_post)
    end

    Repo.insert!(%ShadowPost{
      rowid: post.id,
      title_embedding: title_embedding,
      summary_embedding: summary_embedding,
      description_embedding: description_embedding
    })
  end
end
```

After running all of the migrations invoke the above job in batches to populate the embeddings.
```
import Ecto.Query
alias MyApp.Jobs.Post
alias MyApp.Jobs.ShadowPost
alias MyApp.Repo

posts = MyApp.Repo.all(from p in Post, limit: 3000, offset: 2000)

post_urls = Enum.map(posts, fn p -> p.job_url end)

post_urls
|> Enum.chunk_every(4)
|> Enum.each(fn chunk ->
  Oban.insert(
    MyApp.Embed.new(%{
      "job_urls" => chunk
    })
  )
end)
```
