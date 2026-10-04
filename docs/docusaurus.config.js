import { themes as prismThemes } from 'prism-react-renderer';

const config = {
  title: 'VISR', tagline: 'A portable spatial interface runtime',
  favicon: 'img/favicon.svg', url: 'https://visr.dev', baseUrl: '/',
  organizationName: 'CamilaBasualdoCibils', projectName: 'visr',
  onBrokenLinks: 'throw',
  markdown: { mermaid: true, hooks: { onBrokenMarkdownLinks: 'throw' } },
  themes: ['@docusaurus/theme-mermaid'],
  presets: [['classic', {
    docs: { path: './pages', sidebarPath: './sidebars.js', routeBasePath: 'docs' },
    blog: false, pages: { path: 'src/pages' },
    theme: { customCss: './src/css/custom.css' },
  }]],
  themeConfig: {
    image: 'img/visr-social-card.svg',
    colorMode: { defaultMode: 'dark', respectPrefersColorScheme: true },
    navbar: {
      title: 'VISR', logo: { alt: 'VISR mark', src: 'img/visr-mark.svg' },
      items: [
        { type: 'docSidebar', sidebarId: 'docsSidebar', position: 'left', label: 'Documentation' },
        { href: 'https://github.com/CamilaBasualdoCibils/visr', label: 'Source', position: 'right' },
      ],
    },
    footer: { style: 'dark', links: [{ title: 'Explore', items: [
      { label: 'Design', to: '/docs/design' },
      { label: 'Architecture', to: '/docs/architecture' },
      { label: 'Hardware', to: '/docs/hardware' },
    ] }], copyright: `Copyright © ${new Date().getFullYear()} VISR contributors.` },
    prism: { theme: prismThemes.github, darkTheme: prismThemes.dracula },
    mermaid: { theme: { light: 'neutral', dark: 'dark' } },
  },
};

export default config;
