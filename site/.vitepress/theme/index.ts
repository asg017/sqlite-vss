// https://vitepress.dev/guide/custom-theme
import { h } from "vue";
import Theme from "vitepress/theme";
import "./style.css";

export default {
  ...Theme,
  Layout: () => {
    return h(Theme.Layout, null, {
      //"home-hero-image": () => h("div", null, "yo"),
    });
  },
  enhanceApp({ app, router, siteData }) {
    // ...
  },
};
