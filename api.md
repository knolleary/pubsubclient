---
layout: default
title: API Documentation
---

### Library version: 2.8

{% assign sorted = site.posts | reverse %}
{% assign currentType = "" %}
{% for post in sorted %}
  {% if post.tags contains 'api' or post.tags contains 'docs' %}
  {% unless post.type == currentType%}
    {%if currentType != "" %}</ul>{%endif%}
<h3>{{post.type | capitalize}}</h3>
    {% assign currentType = post.type %}
<ul class="post-list">
  {% endunless %}
  <li><a href="#{{post.slug}}">{%if post.tags contains 'api'
      %}{% if post.returns %}<span class="methodreturn">{{ post.returns.type }}</span> {% endif %}<span class="methodname">{{ post.name }} </span> <span class="methodparams">({% include paramsList.html params=post.params %})</span>{%
      else %}{{ post.title }}{%endif%}</a></li>
  {% endif %}
{% endfor %}
</ul>

---

{% for post in sorted %}
  {% if post.tags contains 'api' or post.tags contains 'docs'%}

  <section class="method" id="{{post.slug}}">
{% if post.tags contains 'api' %}
      <h4>{% if post.returns %}<span class="methodreturn">{{ post.returns.type }}</span> {% endif %}<span class="methodname">{{ post.name }}</span> <span class="methodparams">({% include paramsList.html params=post.params %})</span></h4>
      {{ post.content }}
      {% if post.params %}
      <h5>Parameters</h5>
      <ul>
       {% include paramsListLong.html params=post.params %}
      </ul>
      {% endif %}
      {% if post.returns and post.returns.values %}
      <h5>Returns</h5>
      <ul>
      {% for return in post.returns.values %}<li>{% if return.value %}<span class="methodreturnvalue">{{ return.value }}</span> - {%endif%}{{ return.description}}</li>{% endfor %}
      </ul>
      {% endif %}
{% else %}
    <h4>{{post.title}}</h4>
    {{post.content}}
{% endif %}
  </section>

  {% endif %}
{% endfor %}
