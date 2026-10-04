const sidebars = {
  docsSidebar: [
    'getting-started/index',
    { type: 'category', label: 'Design', link: { type: 'doc', id: 'design/index' }, items: ['design/vision', 'design/goals', 'design/inspirations'] },
    { type: 'category', label: 'Concepts', link: { type: 'doc', id: 'concepts/index' }, items: [] },
    'architecture/index', 'hardware/index', 'applications/index', 'internals/index',
  ],
};
export default sidebars;
