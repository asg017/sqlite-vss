import { defineConfig, DefaultTheme, HeadConfig } from "vitepress";
import { readFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const PROJECT = "sqlite-vss";
const description =
  "A SQLite extension for efficient vector search, based on Faiss!";

const VERSION = readFileSync(
  join(dirname(fileURLToPath(import.meta.url)), "..", "..", "VERSION"),
  "utf8"
);

function head(): HeadConfig[] {
  return [
    [
      "link",
      {
        rel: "shortcut icon",
        type: "image/svg+xml",
        href: "favicon.svg",
      },
    ],
    [
      "script",
      {
        defer: "",
        "data-domain": "alexgarcia.xyz/sqlite-vss",
        src: "https://plausible.io/js/script.js",
      },
    ],
  ];
}

const guides = {
  text: "Guides",
  items: [
    //{ text: "OpenAI's Embedding API", link: "/openai-python" },
    //{ text: "openai-to-sqlite", link: "/openai-to-sqlite" },
    //{ text: "Sentence Transformers", link: "/sentence-transformers-python",},
    //{ text: "HuggingFace Inference API", link: "/huggingface",},
    //{ text: "Cohere's Embedding API", link: "/cohere" },
  ],
};

function nav(): DefaultTheme.NavItem[] {
  return [
    { text: "Home", link: "/" },
    //guides,
    { text: "API Reference", link: "/api-reference" },
    { text: "♥ Sponsor", link: "https://github.com/sponsors/asg017" },
    {
      text: `v${VERSION}`,
      items: [
        {
          text: "Github Release",
          link: `https://github.com/asg017/${PROJECT}/releases/${VERSION}`,
        },
        {
          text: "Bindings",
          items: [
            {
              text: "Python: PyPi package",
              link: `https://pypi.org/project/${PROJECT}`,
            },
            {
              text: "Node.js: NPM package",
              link: `https://www.npmjs.com/package/${PROJECT}`,
            },
            {
              text: "Deno: deno.land/x module",
              link: `https://deno.land/x/${PROJECT.replace("-", "_")}`,
            },
            {
              text: "Ruby: Ruby gem",
              link: `https://rubygems.org/gems/${PROJECT}`,
            },
            {
              text: "Elixir: Hex package",
              link: `https://crates.io/crates/${PROJECT.replace("-", "_")}`,
            },
            {
              text: "Rust: Cargo crate",
              link: `https://crates.io/crates/${PROJECT}`,
            },
            {
              text: "Golang: Go module",
              link: `https://pkg.go.dev/github.com/asg017/${PROJECT}/bindings/go`,
            },
            {
              text: "Datasette: Plugin",
              link: `https://datasette.io/plugins/datasette-${PROJECT}`,
            },
            {
              text: "sqlite-utils: Plugin",
              link: `https://datasette.io/plugins/datasette-${PROJECT}`,
            },
          ],
        },
      ],
    },
  ];
}

function sidebar(): DefaultTheme.Sidebar {
  return [
    {
      text: "Getting Started",
      collapsed: false,
      items: [
        {
          text: "Introduction",
          link: "/getting-started#introduction",
        },
        { text: "Installing", link: "/getting-started#installing" },
        { text: "Basic Example", link: "/getting-started#basic-example" },
        { text: "Next Steps", link: "/getting-started#next-steps" },
      ],
    },
    {
      text: "Using with...",
      collapsed: true,
      items: [
        { text: "Python", link: "/python" },
        { text: "Datasette", link: "/datasette" },
        { text: "Node.js", link: "/nodejs" },
        { text: "Deno", link: "/deno" },
        { text: "Ruby", link: "/ruby" },
        { text: "Elixir", link: "/elixir" },
        { text: "Rust", link: "/rust" },
        { text: "Go", link: "/go" },
        { text: "Loadable Extension", link: "/loadable" },
        { text: "Turso", link: "/turso" },
      ],
    },
    //guides,
    {
      text: "Comparisons with...",
      link: "/compare",
    },
    {
      text: "Documentation",
      items: [
        { text: "Building from Source", link: "/building-source" },
        { text: "API Reference", link: "/api-reference" },
      ],
    },
    {
      text: "See also",
      items: [
        {
          text: "sqlite-ecosystem",
          link: "https://github.com/asg017/sqlite-ecosystem",
        },
        { text: "sqlite-http", link: "https://github.com/asg017/sqlite-http" },
        { text: "sqlite-xsv", link: "https://github.com/asg017/sqlite-xsv" },
      ],
    },
  ];
}
export default defineConfig({
  title: PROJECT,
  description,
  lastUpdated: true,
  head: head(),
  base: "/sqlite-vss/",
  themeConfig: {
    nav: nav(),
    sidebar: sidebar(),
    footer: {
      message: "MIT License",
      copyright: "Copyright © 2023 Alex Garcia",
    },
    outline: "deep",
    search: {
      provider: "local",
    },
    socialLinks: [
      { icon: "github", link: `https://github.com/asg017/${PROJECT}` },
      { icon: "discord", link: "https://discord.gg/F9BZdpABMN" },
    ],
    editLink: {
      pattern: `https://github.com/asg017/${PROJECT}/edit/main/site/:path`,
    },
  },
  rewrites: {
    "using/:pkg.md": ":pkg.md",
    "guides/:pkg.md": ":pkg.md",
  },
  markdown: {
    languages: [
      {
        id: "sqlite",
        scopeName: "source.sqlite",
        grammar: JSON.parse(readFileSync("sql.tmLanguage.json", "utf8")),
        path: "sqlite",
        aliases: ["sqlite"],
      },
    ],
  },
});
